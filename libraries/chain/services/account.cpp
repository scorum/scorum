#include <scorum/chain/database/database.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/rewards_math/formulas.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

namespace scorum {
namespace chain {

dbs_account::dbs_account(database& db)
    : base_service_type(db)
    , _dgp_svc(db.dynamic_global_property_service())
    , _witness_svc(db.witness_service())
{
}

const account_object& dbs_account::get(const account_id_type& account_id) const
{
    try
    {
        return get_by<by_id>(account_id);
    }
    FC_CAPTURE_AND_RETHROW((account_id))
}

const account_object& dbs_account::get_account(const account_name_type& name) const
{
    try
    {
        return get_by<by_name>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

bool dbs_account::is_exists(const account_name_type& name) const
{
    return find_by<by_name>(name) != nullptr;
}

const account_authority_object& dbs_account::get_account_authority(const account_name_type& name) const
{
    try
    {
        return db_impl().get<account_authority_object, by_account>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

void dbs_account::check_account_existence(const account_name_type& name,
                                          const optional<const char*>& context_type_name) const
{
    auto acc = find_by<by_name>(name);
    if (context_type_name.valid())
    {
        FC_ASSERT(acc != nullptr, "\"${1}\" \"${2}\" must exist.", ("1", *context_type_name)("2", name));
    }
    else
    {
        FC_ASSERT(acc != nullptr, "Account \"${1}\" must exist.", ("1", name));
    }
}

void dbs_account::check_account_existence(const account_authority_map& names,
                                          const optional<const char*>& context_type_name) const
{
    for (const auto& a : names)
    {
        check_account_existence(a.first, context_type_name);
    }
}

const account_object& dbs_account::create_initial_account(const account_name_type& new_account_name,
                                                          const public_key_type& memo_key,
                                                          const asset& balance,
                                                          const std::string& json_metadata)
{
    FC_ASSERT(new_account_name.size() > 0, "Account 'name' should not be empty.");
    FC_ASSERT(balance.symbol() == SCORUM_SYMBOL, "Invalid asset type (symbol) for balance.");

    authority owner;
    if (memo_key != public_key_type())
    {
        owner.add_authority(memo_key, 1);
        owner.weight_threshold = 1;
    }
    const auto& new_account
        = _create_account_objects(new_account_name, account_name_type(), memo_key, json_metadata, owner, owner, owner);

    update(new_account, [&](account_object& acc) {
        acc.created_by_genesis = true;
        acc.balance = balance;
    });

    return new_account;
}

const account_object& dbs_account::create_account(const account_name_type& new_account_name,
                                                  const account_name_type& creator_name,
                                                  const public_key_type& memo_key,
                                                  const std::string& json_metadata,
                                                  const authority& owner,
                                                  const authority& active,
                                                  const authority& posting,
                                                  const asset& fee)
{
    FC_ASSERT(fee.symbol() == SCORUM_SYMBOL, "invalid asset type (symbol)");

    if (fee.amount > 0)
        decrease_balance(get_account(creator_name), fee);

    const auto& new_account
        = _create_account_objects(new_account_name, creator_name, memo_key, json_metadata, owner, active, posting);

    if (fee.amount > 0)
        create_scorumpower(new_account, fee);

    return new_account;
}

const account_object& dbs_account::create_account_with_delegation(const account_name_type& new_account_name,
                                                                  const account_name_type& creator_name,
                                                                  const public_key_type& memo_key,
                                                                  const std::string& json_metadata,
                                                                  const authority& owner,
                                                                  const authority& active,
                                                                  const authority& posting,
                                                                  const asset& fee,
                                                                  const asset& delegation)
{
    FC_ASSERT(fee.symbol() == SCORUM_SYMBOL, "invalid asset type (symbol)");
    FC_ASSERT(delegation.symbol() == SP_SYMBOL, "invalid asset type (symbol)");

    const auto& creator = get_account(creator_name);

    update(creator, [&](account_object& c) { c.delegated_scorumpower += delegation; });

    decrease_balance(creator, fee);

    const auto& new_account
        = _create_account_objects(new_account_name, creator_name, memo_key, json_metadata, owner, active, posting);

    update(new_account, [&](account_object& acc) { acc.received_scorumpower = delegation; });

    if (delegation.amount > 0)
    {
        time_point_sec t = db_impl().head_block_time();

        db_impl().create<scorumpower_delegation_object>([&](scorumpower_delegation_object& vdo) {
            vdo.delegator = creator_name;
            vdo.delegatee = new_account_name;
            vdo.scorumpower = delegation;
            vdo.min_delegation_time = t + SCORUM_CREATE_ACCOUNT_DELEGATION_TIME;
        });
    }

    if (fee.amount > 0)
        create_scorumpower(new_account, fee);

    return new_account;
}

const account_object& dbs_account::create_account_with_bonus(const account_name_type& new_account_name,
                                                             const account_name_type& creator_name,
                                                             const public_key_type& memo_key,
                                                             const std::string& json_metadata,
                                                             const authority& owner,
                                                             const authority& active,
                                                             const authority& posting,
                                                             const asset& bonus)
{
    FC_ASSERT(bonus.symbol() == SCORUM_SYMBOL, "invalid asset type (symbol)");

    const auto& new_account
        = _create_account_objects(new_account_name, creator_name, memo_key, json_metadata, owner, active, posting);

    if (bonus.amount > 0)
    {
        create_scorumpower(new_account, bonus);
    }

    return new_account;
}

void dbs_account::update_acount(const account_object& account,
                                const account_authority_object& account_authority,
                                const public_key_type& memo_key,
                                const std::string& json_metadata,
                                const optional<authority>& owner,
                                const optional<authority>& active,
                                const optional<authority>& posting)
{
    time_point_sec t = db_impl().head_block_time();

    update(account, [&](account_object& acc) {
        if (memo_key != public_key_type())
            acc.memo_key = memo_key;

        if ((active || owner) && acc.active_challenged)
        {
            acc.active_challenged = false;
            acc.last_active_proved = t;
        }

        acc.last_account_update = t;

#ifndef IS_LOW_MEM
        if (json_metadata.size() > 0)
            fc::from_string(acc.json_metadata, json_metadata);
#endif
    });

    if (active || posting)
    {
        db_impl().modify(account_authority, [&](account_authority_object& auth) {
            if (active)
                auth.active = *active;
            if (posting)
                auth.posting = *posting;
        });
    }
}

void dbs_account::update_owner_authority(const account_object& account, const authority& owner_authority)
{
    time_point_sec t = db_impl().head_block_time();

    db_impl().create<owner_authority_history_object>([&](owner_authority_history_object& hist) {
        hist.account = account.name;
        hist.previous_owner_authority = db_impl().get<account_authority_object, by_account>(account.name).owner;
        hist.last_valid_time = t;
    });

    db_impl().modify(db_impl().get<account_authority_object, by_account>(account.name),
                     [&](account_authority_object& auth) {
                         auth.owner = owner_authority;
                         auth.last_owner_update = t;
                     });
}

void dbs_account::increase_balance(const account_object& account, const asset& amount)
{
    // clang-format off
    FC_ASSERT(amount.symbol() == SCORUM_SYMBOL, "invalid asset type (symbol)");
    update(account, [&](account_object& acnt) { acnt.balance += amount; });

    _dgp_svc.update([&](dynamic_global_property_object& props) {
        props.circulating_capital += amount;
    });
    // clang-format on
}

void dbs_account::increase_balance(account_name_type account_name, const asset& amount)
{
    const auto& acc = get_account(account_name);
    increase_balance(acc, amount);
}

void dbs_account::decrease_balance(const account_object& account, const asset& amount)
{
    increase_balance(account, -amount);
}

void dbs_account::increase_pending_balance(const account_object& account, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SCORUM_SYMBOL, "invalid asset type (symbol)");
    update(account, [&](account_object& acnt) { acnt.active_sp_holders_pending_scr_reward += amount; });

    _dgp_svc.update([&](dynamic_global_property_object& props) { props.total_pending_scr += amount; });
}

void dbs_account::decrease_pending_balance(const account_object& account, const asset& amount)
{
    increase_pending_balance(account, -amount);
}

void dbs_account::increase_scorumpower(const account_object& account, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SP_SYMBOL, "invalid asset type (symbol)");
    update(account, [&](account_object& a) { a.scorumpower += amount; });

    _dgp_svc.update([&](dynamic_global_property_object& props) {
        props.circulating_capital += asset(amount.amount, SCORUM_SYMBOL);
        props.total_scorumpower += amount;
    });

    adjust_proxied_witness_votes(account, amount.amount);
}

void dbs_account::decrease_scorumpower(const account_object& account, const asset& amount)
{
    increase_scorumpower(account, -amount);
}

void dbs_account::increase_pending_scorumpower(const account_object& account, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SP_SYMBOL, "invalid asset type (symbol)");
    update(account, [&](account_object& acnt) { acnt.active_sp_holders_pending_sp_reward += amount; });

    _dgp_svc.update([&](dynamic_global_property_object& props) { props.total_pending_sp += amount; });
}

void dbs_account::decrease_pending_scorumpower(const account_object& account, const asset& amount)
{
    increase_pending_scorumpower(account, -amount);
}

const asset dbs_account::create_scorumpower(const account_object& to_account, const asset& scorum)
{
    try
    {
        asset new_scorumpower = asset(scorum.amount, SP_SYMBOL);

        increase_scorumpower(to_account, new_scorumpower);

        return new_scorumpower;
    }
    FC_CAPTURE_AND_RETHROW((to_account.name)(scorum))
}

void dbs_account::increase_delegated_scorumpower(const account_object& account, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SP_SYMBOL, "invalid asset type (symbol)");
    update(account, [&](account_object& a) { a.delegated_scorumpower += amount; });
}

void dbs_account::increase_received_scorumpower(const account_object& account, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SP_SYMBOL, "invalid asset type (symbol)");
    update(account, [&](account_object& a) { a.received_scorumpower += amount; });
}

void dbs_account::decrease_received_scorumpower(const account_object& account, const asset& amount)
{
    increase_received_scorumpower(account, -amount);
}

void dbs_account::drop_challenged(const account_object& account)
{
    time_point_sec t = db_impl().head_block_time();

    if (account.active_challenged)
    {
        update(account, [&](account_object& a) {
            a.active_challenged = false;
            a.last_active_proved = t;
        });
    }
}

void dbs_account::prove_authority(const account_object& account, bool require_owner)
{
    time_point_sec t = db_impl().head_block_time();

    update(account, [&](account_object& a) {
        a.active_challenged = false;
        a.last_active_proved = t;
        if (require_owner)
        {
            a.owner_challenged = false;
            a.last_owner_proved = t;
        }
    });
}

void dbs_account::increase_witnesses_voted_for(const account_object& account)
{
    update(account, [&](account_object& a) { a.witnesses_voted_for++; });
}

void dbs_account::decrease_witnesses_voted_for(const account_object& account)
{
    update(account, [&](account_object& a) { a.witnesses_voted_for--; });
}

void dbs_account::add_post(const account_object& author_account, const account_name_type& parent_author_name)
{
    time_point_sec t = db_impl().head_block_time();

    update(author_account, [&](account_object& a) {
        if (parent_author_name == SCORUM_ROOT_POST_PARENT_ACCOUNT)
        {
            a.last_root_post = t;
        }
        a.last_post = t;
    });
}

void dbs_account::update_voting_power(const account_object& account, uint16_t voting_power)
{
    if (voting_power < account.voting_power)
    {
        update_active_sp_holders_cashout_time(account);
    }

    time_point_sec t = db_impl().head_block_time();

    update(account, [&](account_object& a) {
        a.voting_power = voting_power;
        a.last_vote_time = t;
        a.voting_power_restoring_time = scorum::rewards_math::calculate_expected_restoring_time(
            voting_power, t, SCORUM_VOTE_REGENERATION_SECONDS);
        a.vote_reward_competitive_sp = a.effective_scorumpower();
    });
}

void dbs_account::update_active_sp_holders_cashout_time(const account_object& account)
{
    if (account.active_sp_holders_cashout_time == fc::time_point_sec::maximum())
    {
        update(account, [&](account_object& a) {
            fc::time_point_sec cashout_time = db_impl().head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD;
            a.active_sp_holders_cashout_time = cashout_time;
        });
    }
}

void dbs_account::create_account_recovery(const account_name_type& account_to_recover,
                                          const authority& new_owner_authority)
{
    time_point_sec t = db_impl().head_block_time();

    const auto& recovery_request_idx
        = db_impl().get_index<account_recovery_request_index>().indices().get<by_account>();
    auto request = recovery_request_idx.find(account_to_recover);

    if (request == recovery_request_idx.end()) // New Request
    {
        FC_ASSERT(!new_owner_authority.is_impossible(), "Cannot recover using an impossible authority.");
        FC_ASSERT(new_owner_authority.weight_threshold, "Cannot recover using an open authority.");

        check_account_existence(new_owner_authority.account_auths);

        db_impl().create<account_recovery_request_object>([&](account_recovery_request_object& req) {
            req.account_to_recover = account_to_recover;
            req.new_owner_authority = new_owner_authority;
            req.expires = t + SCORUM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
        });
    }
    else if (new_owner_authority.weight_threshold == 0) // Cancel Request if authority is open
    {
        db_impl().remove(*request);
    }
    else // Change Request
    {
        FC_ASSERT(!new_owner_authority.is_impossible(), "Cannot recover using an impossible authority.");

        check_account_existence(new_owner_authority.account_auths);

        db_impl().modify(*request, [&](account_recovery_request_object& req) {
            req.new_owner_authority = new_owner_authority;
            req.expires = t + SCORUM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
        });
    }
}

void dbs_account::submit_account_recovery(const account_object& account_to_recover,
                                          const authority& new_owner_authority,
                                          const authority& recent_owner_authority)
{
    time_point_sec t = db_impl().head_block_time();

    const auto& recovery_request_idx
        = db_impl().get_index<account_recovery_request_index>().indices().get<by_account>();
    auto request = recovery_request_idx.find(account_to_recover.name);

    FC_ASSERT(request != recovery_request_idx.end(), "There are no active recovery requests for this account.");
    FC_ASSERT(request->new_owner_authority == new_owner_authority,
              "New owner authority does not match recovery request.");

    const auto& recent_auth_idx = db_impl().get_index<owner_authority_history_index>().indices().get<by_account>();
    auto hist = recent_auth_idx.lower_bound(account_to_recover.name);
    bool found = false;

    while (hist != recent_auth_idx.end() && hist->account == account_to_recover.name && !found)
    {
        found = hist->previous_owner_authority == recent_owner_authority;
        if (found)
            break;
        ++hist;
    }

    FC_ASSERT(found, "Recent authority not found in authority history.");

    db_impl().remove(*request); // Remove first, update_owner_authority may invalidate iterator
    update_owner_authority(account_to_recover, new_owner_authority);
    update(account_to_recover, [&](account_object& a) { a.last_account_recovery = t; });
}

void dbs_account::change_recovery_account(const account_object& account_to_recover,
                                          const account_name_type& new_recovery_account_name)
{
    time_point_sec t = db_impl().head_block_time();

    const auto& change_recovery_idx
        = db_impl().get_index<change_recovery_account_request_index>().indices().get<by_account>();
    auto request = change_recovery_idx.find(account_to_recover.name);

    if (request == change_recovery_idx.end()) // New request
    {
        db_impl().create<change_recovery_account_request_object>([&](change_recovery_account_request_object& req) {
            req.account_to_recover = account_to_recover.name;
            req.recovery_account = new_recovery_account_name;
            req.effective_on = t + SCORUM_OWNER_AUTH_RECOVERY_PERIOD;
        });
    }
    else if (account_to_recover.recovery_account != new_recovery_account_name) // Change existing request
    {
        db_impl().modify(*request, [&](change_recovery_account_request_object& req) {
            req.recovery_account = new_recovery_account_name;
            req.effective_on = t + SCORUM_OWNER_AUTH_RECOVERY_PERIOD;
        });
    }
    else // Request exists and changing back to current recovery account
    {
        db_impl().remove(*request);
    }
}

void dbs_account::update_voting_proxy(const account_object& account, const optional<account_object>& proxy_account)
{
    /// remove all current votes
    std::array<share_type, SCORUM_MAX_PROXY_RECURSION_DEPTH + 1> delta;
    delta[0] = -account.scorumpower.amount;
    for (int i = 0; i < SCORUM_MAX_PROXY_RECURSION_DEPTH; ++i)
        delta[i + 1] = -account.proxied_vsf_votes[i];

    adjust_proxied_witness_votes(account, delta);

    if (proxy_account.valid())
    {
        flat_set<account_id_type> proxy_chain({ account.id, (*proxy_account).id });
        proxy_chain.reserve(SCORUM_MAX_PROXY_RECURSION_DEPTH + 1);

        /// check for proxy loops and fail to update the proxy if it would create a loop
        auto cprox = &(*proxy_account);
        while (cprox->proxy.size() != 0)
        {
            const auto next_proxy = get_account(cprox->proxy);
            FC_ASSERT(proxy_chain.insert(next_proxy.id).second, "This proxy would create a proxy loop.");
            cprox = &next_proxy;
            FC_ASSERT(proxy_chain.size() <= SCORUM_MAX_PROXY_RECURSION_DEPTH, "Proxy chain is too long.");
        }

        /// clear all individual vote records
        clear_witness_votes(account);

        update(account, [&](account_object& a) { a.proxy = (*proxy_account).name; });

        /// add all new votes
        for (int i = 0; i <= SCORUM_MAX_PROXY_RECURSION_DEPTH; ++i)
            delta[i] = -delta[i];
        adjust_proxied_witness_votes(account, delta);
    }
    else
    { /// we are clearing the proxy which means we simply update the account
        update(account, [&](account_object& a) { a.proxy = account_name_type(SCORUM_PROXY_TO_SELF_ACCOUNT); });
    }
}

void dbs_account::clear_witness_votes(const account_object& account)
{
    const auto& vidx = db_impl().get_index<witness_vote_index>().indices().get<by_account_witness>();
    auto itr = vidx.lower_bound(boost::make_tuple(account.id, witness_id_type()));
    while (itr != vidx.end() && itr->account == account.id)
    {
        const auto& current = *itr;
        ++itr;
        db_impl().remove(current);
    }

    update(account, [&](account_object& acc) { acc.witnesses_voted_for = 0; });
}

void dbs_account::adjust_proxied_witness_votes(
    const account_object& account, const std::array<share_type, SCORUM_MAX_PROXY_RECURSION_DEPTH + 1>& delta, int depth)
{
    if (account.proxy != SCORUM_PROXY_TO_SELF_ACCOUNT)
    {
        /// nested proxies are not supported, vote will not propagate
        if (depth >= SCORUM_MAX_PROXY_RECURSION_DEPTH)
            return;

        const auto& proxy = get_account(account.proxy);

        update(proxy, [&](account_object& a) {
            for (int i = SCORUM_MAX_PROXY_RECURSION_DEPTH - depth - 1; i >= 0; --i)
            {
                a.proxied_vsf_votes[i + depth] += delta[i];
            }
        });

        adjust_proxied_witness_votes(proxy, delta, depth + 1);
    }
    else
    {
        share_type total_delta = 0;
        for (int i = SCORUM_MAX_PROXY_RECURSION_DEPTH - depth; i >= 0; --i)
            total_delta += delta[i];

        _witness_svc.adjust_witness_votes(account, total_delta);
    }
}

void dbs_account::adjust_proxied_witness_votes(const account_object& account, const share_type& delta, int depth)
{
    if (account.proxy != SCORUM_PROXY_TO_SELF_ACCOUNT)
    {
        /// nested proxies are not supported, vote will not propagate
        if (depth >= SCORUM_MAX_PROXY_RECURSION_DEPTH)
            return;

        const auto& proxy = get_account(account.proxy);

        update(proxy, [&](account_object& a) { a.proxied_vsf_votes[depth] += delta; });

        adjust_proxied_witness_votes(proxy, delta, depth + 1);
    }
    else
    {
        _witness_svc.adjust_witness_votes(account, delta);
    }
}

const account_object& dbs_account::_create_account_objects(const account_name_type& new_account_name,
                                                           const account_name_type& recovery_account,
                                                           const public_key_type& memo_key,
                                                           const std::string& json_metadata,
                                                           const authority& owner,
                                                           const authority& active,
                                                           const authority& posting)
{
    const auto& new_account = create([&](account_object& acc) {
        acc.name = new_account_name;
        acc.recovery_account = recovery_account;
        acc.memo_key = memo_key;
        acc.created = _dgp_svc.head_block_time();
#ifndef IS_LOW_MEM
        fc::from_string(acc.json_metadata, json_metadata);
#endif
    });

    if (memo_key != public_key_type())
    {
        db_impl().create<account_authority_object>([&](account_authority_object& auth) {
            auth.account = new_account_name;
            auth.owner = owner;
            auth.active = active;
            auth.posting = posting;
            auth.last_owner_update = fc::time_point_sec::min();
        });
    }

    return new_account;
}

dbs_account::account_refs_type dbs_account::get_active_sp_holders() const
{
    fc::time_point_sec min_vote_time_for_cashout = _dgp_svc.head_block_time();

    return get_range_by<by_voting_power_restoring_time>(min_vote_time_for_cashout < boost::lambda::_1,
                                                        boost::multi_index::unbounded);
}

void dbs_account::foreach_account(account_call_type&& call) const
{
    foreach_by<by_id>(call);
}
dbs_account::account_refs_type dbs_account::get_by_cashout_time(const fc::time_point_sec& until) const
{
    try
    {
        return get_range_by<by_active_sp_holders_cashout_time>(::boost::multi_index::unbounded,
                                                               ::boost::lambda::_1 <= std::make_tuple(until, ALL_IDS));
    }
    FC_CAPTURE_AND_RETHROW((until))
}

accounts_total dbs_account::accounts_circulating_capital() const
{
    accounts_total totals;

    const auto& account_idx = db_impl().get_index<account_index>().indices().get<by_name>();
    for (auto itr = account_idx.begin(); itr != account_idx.end(); ++itr)
    {
        totals.scr += itr->balance;
        totals.sp += itr->scorumpower;
        totals.pending_scr += itr->active_sp_holders_pending_scr_reward;
        totals.pending_sp += itr->active_sp_holders_pending_sp_reward;

        totals.vsf_votes += (itr->proxy == SCORUM_PROXY_TO_SELF_ACCOUNT
                                 ? itr->witness_vote_weight()
                                 : (SCORUM_MAX_PROXY_RECURSION_DEPTH > 0
                                        ? itr->proxied_vsf_votes[SCORUM_MAX_PROXY_RECURSION_DEPTH - 1]
                                        : itr->scorumpower.amount));
    }

    return totals;
}

} // namespace chain
} // namespace scorum
