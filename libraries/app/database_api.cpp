#include <scorum/app/api_context.hpp>
#include <scorum/app/application.hpp>
#include <scorum/app/database_api.hpp>

#include <scorum/protocol/get_config.hpp>

#include <scorum/rewards_math/formulas.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/atomicswap.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/development_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/escrow.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/withdraw_scorumpower_route.hpp>
#include <scorum/chain/services/witness_schedule.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/advertising_property.hpp>

#include <scorum/chain/schema/committee.hpp>
#include <scorum/chain/schema/proposal_object.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/budget_objects.hpp>
#include <scorum/chain/schema/reward_balancer_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/advertising_property_object.hpp>

#include <scorum/common_api/config_api.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/container/utils.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include <cctype>

#include <cfenv>
#include <iostream>

namespace scorum {
namespace app {

class database_api_impl;

class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
public:
    database_api_impl(const scorum::app::api_context& ctx);
    ~database_api_impl();

    // Subscriptions
    void set_block_applied_callback(std::function<void(const variant& block_id)> cb);

    // Globals
    fc::variant_object get_config() const;
    dynamic_global_property_api_obj get_dynamic_global_properties() const;
    chain_id_type get_chain_id() const;

    // Keys
    std::vector<std::set<std::string>> get_key_references(std::vector<public_key_type> key) const;

    // Accounts
    std::vector<extended_account> get_accounts(const std::vector<std::string>& names) const;
    std::vector<account_id_type> get_account_references(account_id_type account_id) const;
    std::vector<optional<account_api_obj>> lookup_account_names(const std::vector<std::string>& account_names) const;
    std::set<std::string> lookup_accounts(const std::string& lower_bound_name, uint32_t limit) const;
    uint64_t get_account_count() const;

    // Budgets
    template <typename BudgetService>
    std::vector<budget_api_obj> get_budgets(BudgetService& budget_service, const std::set<std::string>& names) const
    {
        FC_ASSERT(names.size() <= get_api_config(API_DATABASE).max_budgets_list_size,
                  "names size must be less or equal than ${1}",
                  ("1", get_api_config(API_DATABASE).max_budgets_list_size));

        std::vector<budget_api_obj> results;

        for (const auto& name : names)
        {
            auto budgets = budget_service.get_budgets(name);
            if (results.size() + budgets.size() > get_api_config(API_DATABASE).max_budgets_list_size)
            {
                break;
            }

            for (const auto& budget : budgets)
            {
                results.emplace_back(budget);
            }
        }

        return results;
    }
    template <typename BudgetService>
    std::set<std::string>
    lookup_budget_owners(BudgetService& budget_service, const std::string& lower_bound_name, uint32_t limit) const
    {
        FC_ASSERT(limit <= get_api_config(API_DATABASE).max_budgets_list_size, "limit must be less or equal than ${1}",
                  ("1", get_api_config(API_DATABASE).max_budgets_list_size));

        return budget_service.lookup_budget_owners(lower_bound_name, limit);
    }

    // Atomic Swap
    std::vector<atomicswap_contract_api_obj> get_atomicswap_contracts(const std::string& owner) const;
    atomicswap_contract_info_api_obj
    get_atomicswap_contract(const std::string& from, const std::string& to, const std::string& secret_hash) const;

    // Witnesses
    std::vector<optional<witness_api_obj>> get_witnesses(const std::vector<witness_id_type>& witness_ids) const;
    fc::optional<witness_api_obj> get_witness_by_account(const std::string& account_name) const;
    std::set<account_name_type> lookup_witness_accounts(const std::string& lower_bound_name, uint32_t limit) const;
    uint64_t get_witness_count() const;

    // Committee
    std::set<account_name_type> lookup_registration_committee_members(const std::string& lower_bound_name,
                                                                      uint32_t limit) const;

    std::set<account_name_type> lookup_development_committee_members(const std::string& lower_bound_name,
                                                                     uint32_t limit) const;

    std::vector<proposal_api_obj> lookup_proposals() const;

    // Authority / validation
    std::string get_transaction_hex(const signed_transaction& trx) const;
    std::set<public_key_type> get_required_signatures(const signed_transaction& trx,
                                                      const flat_set<public_key_type>& available_keys) const;
    std::set<public_key_type> get_potential_signatures(const signed_transaction& trx) const;
    bool verify_authority(const signed_transaction& trx) const;
    bool verify_account_authority(const std::string& name_or_id, const flat_set<public_key_type>& signers) const;

    // signal handlers
    void on_applied_block(const chain::signed_block& b);

    std::function<void(const fc::variant&)> _block_applied_callback;

    scorum::chain::database& _db;

    boost::signals2::scoped_connection _block_applied_connection;

    registration_committee_api_obj get_registration_committee() const;
    development_committee_api_obj get_development_committee() const;
    advertising_property_api_obj get_advertising_property() const;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_block_applied_callback(std::function<void(const variant& block_id)> cb)
{
    my->_db.with_read_lock([&]() { my->set_block_applied_callback(cb); });
}

void database_api_impl::on_applied_block(const chain::signed_block& b)
{
    try
    {
        _block_applied_callback(fc::variant(signed_block_header(b)));
    }
    catch (...)
    {
        _block_applied_connection.release();
    }
}

void database_api_impl::set_block_applied_callback(std::function<void(const variant& block_header)> cb)
{
    _block_applied_callback = cb;
    _block_applied_connection = connect_signal(_db.applied_block, *this, &database_api_impl::on_applied_block);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api(const scorum::app::api_context& ctx)
    : my(new database_api_impl(ctx))
    , _app(ctx.app)
{
}

database_api::~database_api()
{
}

database_api_impl::database_api_impl(const scorum::app::api_context& ctx)
    : _db(*ctx.app.chain_database())
{
    wlog("creating database api ${x}", ("x", int64_t(this)));
}

database_api_impl::~database_api_impl()
{
    elog("freeing database api ${x}", ("x", int64_t(this)));
}

void database_api::on_api_startup()
{
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////=

fc::variant_object database_api::get_config() const
{
    return my->_db.with_read_lock([&]() { return my->get_config(); });
}

fc::variant_object database_api_impl::get_config() const
{
    return scorum::protocol::get_config();
}

dynamic_global_property_api_obj database_api::get_dynamic_global_properties() const
{
    return my->_db.with_read_lock([&]() { return my->get_dynamic_global_properties(); });
}

dynamic_global_property_api_obj database_api_impl::get_dynamic_global_properties() const
{
    dynamic_global_property_api_obj gpao;
    gpao = _db.obtain_service<dbs_dynamic_global_property>().get();

    if (_db.has_index<witness::reserve_ratio_index>())
    {
        const auto& r = _db.find(witness::reserve_ratio_id_type());

        if (BOOST_LIKELY(r != nullptr))
        {
            gpao = *r;
        }
    }

    gpao.registration_pool_balance = _db.obtain_service<dbs_registration_pool>().get().balance;
    gpao.fund_budget_balance = _db.obtain_service<dbs_fund_budget>().get().balance;
    gpao.reward_pool_balance = _db.obtain_service<dbs_content_reward_scr>().get().balance;
    gpao.content_reward_scr_balance = _db.obtain_service<dbs_content_reward_fund_scr>().get().activity_reward_balance;
    gpao.content_reward_sp_balance = _db.obtain_service<dbs_content_reward_fund_sp>().get().activity_reward_balance;

    return gpao;
}

chain_id_type database_api::get_chain_id() const
{
    return my->_db.with_read_lock([&]() { return my->get_chain_id(); });
}

chain_id_type database_api_impl::get_chain_id() const
{
    return _db.get_chain_id();
}

witness_schedule_api_obj database_api::get_witness_schedule() const
{
    return my->_db.with_read_lock([&]() { return my->_db.get(witness_schedule_id_type()); });
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::vector<std::set<std::string>> database_api::get_key_references(std::vector<public_key_type> key) const
{
    return my->_db.with_read_lock([&]() { return my->get_key_references(key); });
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
std::vector<std::set<std::string>> database_api_impl::get_key_references(std::vector<public_key_type> keys) const
{
    FC_ASSERT(false, "database_api::get_key_references has been deprecated. Please use "
                     "account_by_key_api::get_key_references instead.");
    std::vector<std::set<std::string>> final_result;
    return final_result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::vector<extended_account> database_api::get_accounts(const std::vector<std::string>& names) const
{
    return my->_db.with_read_lock([&]() { return my->get_accounts(names); });
}

std::vector<extended_account> database_api_impl::get_accounts(const std::vector<std::string>& names) const
{
    const auto& idx = _db.get_index<account_index>().indices().get<by_name>();
    const auto& vidx = _db.get_index<witness_vote_index>().indices().get<by_account_witness>();
    std::vector<extended_account> results;

    for (auto name : names)
    {
        auto itr = idx.find(name);
        if (itr != idx.end())
        {
            extended_account api_obj(*itr, _db);
            api_obj.voting_power = rewards_math::calculate_restoring_power(
                api_obj.voting_power, _db.head_block_time(), api_obj.last_vote_time, SCORUM_VOTE_REGENERATION_SECONDS);
            results.push_back(std::move(api_obj));

            auto vitr = vidx.lower_bound(boost::make_tuple(itr->id, witness_id_type()));
            while (vitr != vidx.end() && vitr->account == itr->id)
            {
                results.back().witness_votes.insert(_db.get(vitr->witness).owner);
                ++vitr;
            }
        }
    }

    return results;
}

std::vector<account_id_type> database_api::get_account_references(account_id_type account_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_account_references(account_id); });
}

std::vector<account_id_type> database_api_impl::get_account_references(account_id_type account_id) const
{
    /*const auto& idx = _db.get_index<account_index>();
    const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
    const auto& refs = aidx.get_secondary_index<scorum::chain::account_member_index>();
    auto itr = refs.account_to_account_memberships.find(account_id);
    std::vector<account_id_type> result;

    if( itr != refs.account_to_account_memberships.end() )
    {
       result.reserve( itr->second.size() );
       for( auto item : itr->second ) result.push_back(item);
    }
    return result;*/
    FC_ASSERT(false, "database_api::get_account_references --- Needs to be refactored for scorum.");
}

std::vector<optional<account_api_obj>>
database_api::lookup_account_names(const std::vector<std::string>& account_names) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_account_names(account_names); });
}

std::vector<optional<account_api_obj>>
database_api_impl::lookup_account_names(const std::vector<std::string>& account_names) const
{
    std::vector<optional<account_api_obj>> result;
    result.reserve(account_names.size());
    for (auto& name : account_names)
    {
        auto itr = _db.find<account_object, by_name>(name);

        if (itr)
        {
            result.push_back(account_api_obj(*itr, _db));
        }
        else
        {
            result.push_back(optional<account_api_obj>());
        }
    }

    return result;
}

std::set<std::string> database_api::lookup_accounts(const std::string& lower_bound_name, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_accounts(lower_bound_name, limit); });
}

std::set<std::string> database_api_impl::lookup_accounts(const std::string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= get_api_config(API_DATABASE).lookup_limit);
    const auto& accounts_by_name = _db.get_index<account_index>().indices().get<by_name>();
    std::set<std::string> result;

    for (auto itr = accounts_by_name.lower_bound(lower_bound_name); limit-- && itr != accounts_by_name.end(); ++itr)
    {
        result.insert(itr->name);
    }

    return result;
}

uint64_t database_api::get_account_count() const
{
    return my->_db.with_read_lock([&]() { return my->get_account_count(); });
}

uint64_t database_api_impl::get_account_count() const
{
    return _db.get_index<account_index>().indices().size();
}

std::vector<owner_authority_history_api_obj> database_api::get_owner_history(const std::string& account) const
{
    return my->_db.with_read_lock([&]() {
        std::vector<owner_authority_history_api_obj> results;

        const auto& hist_idx = my->_db.get_index<owner_authority_history_index>().indices().get<by_account>();
        auto itr = hist_idx.lower_bound(account);

        while (itr != hist_idx.end() && itr->account == account)
        {
            results.push_back(owner_authority_history_api_obj(*itr));
            ++itr;
        }

        return results;
    });
}

optional<account_recovery_request_api_obj> database_api::get_recovery_request(const std::string& account) const
{
    return my->_db.with_read_lock([&]() {
        optional<account_recovery_request_api_obj> result;

        const auto& rec_idx = my->_db.get_index<account_recovery_request_index>().indices().get<by_account>();
        auto req = rec_idx.find(account);

        if (req != rec_idx.end())
            result = account_recovery_request_api_obj(*req);

        return result;
    });
}

optional<escrow_api_obj> database_api::get_escrow(const std::string& from, uint32_t escrow_id) const
{
    return my->_db.with_read_lock([&]() {
        optional<escrow_api_obj> result;

        try
        {
            result = my->_db.obtain_service<dbs_escrow>().get(from, escrow_id);
        }
        catch (...)
        {
        }

        return result;
    });
}

std::vector<withdraw_route> database_api::get_withdraw_routes(const std::string& account,
                                                              withdraw_route_type type) const
{
    return my->_db.with_read_lock([&]() {
        std::vector<withdraw_route> result;

        const auto& acc = my->_db.obtain_service<chain::dbs_account>().get_account(account);

        if (type == outgoing || type == all)
        {
            const auto& by_route
                = my->_db.get_index<withdraw_scorumpower_route_index>().indices().get<by_withdraw_route>();
            auto route = by_route.lower_bound(acc.id); // TODO: test get_withdraw_routes

            while (route != by_route.end() && is_equal_withdrawable_id(route->from_id, acc.id))
            {
                withdraw_route r;
                r.from_account = account;
                r.to_account = my->_db.get(route->to_id.get<account_id_type>()).name;
                r.percent = route->percent;
                r.auto_vest = route->auto_vest;

                result.push_back(r);

                ++route;
            }
        }

        if (type == incoming || type == all)
        {
            const auto& by_dest = my->_db.get_index<withdraw_scorumpower_route_index>().indices().get<by_destination>();
            auto route = by_dest.lower_bound(acc.id); // TODO: test get_withdraw_routes

            while (route != by_dest.end() && is_equal_withdrawable_id(route->to_id, acc.id))
            {
                withdraw_route r;
                r.from_account = my->_db.get(route->from_id.get<account_id_type>()).name;
                r.to_account = account;
                r.percent = route->percent;
                r.auto_vest = route->auto_vest;

                result.push_back(r);

                ++route;
            }
        }

        return result;
    });
}

optional<account_bandwidth_api_obj> database_api::get_account_bandwidth(const std::string& account,
                                                                        witness::bandwidth_type type) const
{
    optional<account_bandwidth_api_obj> result;

    if (my->_db.has_index<witness::account_bandwidth_index>())
    {
        auto band = my->_db.find<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
            boost::make_tuple(account, type));
        if (band != nullptr)
            result = *band;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::vector<optional<witness_api_obj>>
database_api::get_witnesses(const std::vector<witness_id_type>& witness_ids) const
{
    return my->_db.with_read_lock([&]() { return my->get_witnesses(witness_ids); });
}

std::vector<optional<witness_api_obj>>
database_api_impl::get_witnesses(const std::vector<witness_id_type>& witness_ids) const
{
    std::vector<optional<witness_api_obj>> result;
    result.reserve(witness_ids.size());
    std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                   [this](witness_id_type id) -> optional<witness_api_obj> {
                       if (auto o = _db.find(id))
                           return *o;
                       return {};
                   });
    return result;
}

fc::optional<witness_api_obj> database_api::get_witness_by_account(const std::string& account_name) const
{
    return my->_db.with_read_lock([&]() { return my->get_witness_by_account(account_name); });
}

std::vector<witness_api_obj> database_api::get_witnesses_by_vote(const std::string& from, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() {
        // idump((from)(limit));
        FC_ASSERT(limit <= get_api_config(API_DATABASE).lookup_limit);

        std::vector<witness_api_obj> result;
        result.reserve(limit);

        const auto& name_idx = my->_db.get_index<witness_index>().indices().get<by_name>();
        const auto& vote_idx = my->_db.get_index<witness_index>().indices().get<by_vote_name>();

        auto itr = vote_idx.begin();
        if (from.size())
        {
            auto nameitr = name_idx.find(from);
            FC_ASSERT(nameitr != name_idx.end(), "invalid witness name ${n}", ("n", from));
            itr = vote_idx.iterator_to(*nameitr);
        }

        while (itr != vote_idx.end() && result.size() < limit && itr->votes > 0)
        {
            result.push_back(witness_api_obj(*itr));
            ++itr;
        }
        return result;
    });
}

fc::optional<witness_api_obj> database_api_impl::get_witness_by_account(const std::string& account_name) const
{
    const auto& idx = _db.get_index<witness_index>().indices().get<by_name>();
    auto itr = idx.find(account_name);
    if (itr != idx.end())
        return witness_api_obj(*itr);
    return {};
}

std::set<account_name_type> database_api::lookup_witness_accounts(const std::string& lower_bound_name,
                                                                  uint32_t limit) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_witness_accounts(lower_bound_name, limit); });
}

std::set<account_name_type> database_api_impl::lookup_witness_accounts(const std::string& lower_bound_name,
                                                                       uint32_t limit) const
{
    FC_ASSERT(limit <= get_api_config(API_DATABASE).lookup_limit);
    const auto& witnesses_by_id = _db.get_index<witness_index>().indices().get<by_id>();

    // get all the names and look them all up, sort them, then figure out what
    // records to return.  This could be optimized, but we expect the
    // number of witnesses to be few and the frequency of calls to be rare
    std::set<account_name_type> witnesses_by_account_name;
    for (const witness_api_obj& witness : witnesses_by_id)
        if (witness.owner >= lower_bound_name) // we can ignore anything below lower_bound_name
            witnesses_by_account_name.insert(witness.owner);

    auto end_iter = witnesses_by_account_name.begin();
    while (end_iter != witnesses_by_account_name.end() && limit--)
        ++end_iter;
    witnesses_by_account_name.erase(end_iter, witnesses_by_account_name.end());
    return witnesses_by_account_name;
}

uint64_t database_api::get_witness_count() const
{
    return my->_db.with_read_lock([&]() { return my->get_witness_count(); });
}

uint64_t database_api_impl::get_witness_count() const
{
    return _db.get_index<witness_index>().indices().size();
}

advertising_property_api_obj database_api::get_advertising_property() const
{
    return my->_db.with_read_lock([&]() { return my->get_advertising_property(); });
}

advertising_property_api_obj database_api_impl::get_advertising_property() const
{
    return advertising_property_api_obj(_db.advertising_property_service().get());
}

std::set<account_name_type> database_api::lookup_registration_committee_members(const std::string& lower_bound_name,
                                                                                uint32_t limit) const
{

    return my->_db.with_read_lock([&]() { return my->lookup_registration_committee_members(lower_bound_name, limit); });
}

std::set<account_name_type> database_api::lookup_development_committee_members(const std::string& lower_bound_name,
                                                                               uint32_t limit) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_development_committee_members(lower_bound_name, limit); });
}

std::set<account_name_type>
database_api_impl::lookup_registration_committee_members(const std::string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= get_api_config(API_DATABASE).lookup_limit);

    return committee::lookup_members<registration_committee_member_index>(_db, lower_bound_name, limit);
}

std::set<account_name_type> database_api_impl::lookup_development_committee_members(const std::string& lower_bound_name,
                                                                                    uint32_t limit) const
{
    FC_ASSERT(limit <= get_api_config(API_DATABASE).lookup_limit);

    return committee::lookup_members<dev_committee_member_index>(_db, lower_bound_name, limit);
}

std::vector<proposal_api_obj> database_api::lookup_proposals() const
{
    return my->_db.with_read_lock([&]() { return my->lookup_proposals(); });
}

std::vector<proposal_api_obj> database_api_impl::lookup_proposals() const
{
    const auto& proposals_by_id = _db.get_index<proposal_object_index>().indices().get<by_id>();

    std::vector<proposal_api_obj> proposals;
    for (const proposal_api_obj& obj : proposals_by_id)
    {
        proposals.push_back(obj);
    }

    return proposals;
}

registration_committee_api_obj database_api::get_registration_committee() const
{
    return my->_db.with_read_lock([&]() { return my->get_registration_committee(); });
}

registration_committee_api_obj database_api_impl::get_registration_committee() const
{
    registration_committee_api_obj committee(_db.get(registration_pool_id_type()));

    return committee;
}

development_committee_api_obj database_api::get_development_committee() const
{
    return my->_db.with_read_lock([&]() { return my->get_development_committee(); });
}

development_committee_api_obj database_api_impl::get_development_committee() const
{
    development_committee_api_obj committee;
    committee = _db.get(dev_committee_id_type());

    return committee;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->get_transaction_hex(trx); });
}

std::string database_api_impl::get_transaction_hex(const signed_transaction& trx) const
{
    return fc::to_hex(fc::raw::pack(trx));
}

std::set<public_key_type> database_api::get_required_signatures(const signed_transaction& trx,
                                                                const flat_set<public_key_type>& available_keys) const
{
    return my->_db.with_read_lock([&]() { return my->get_required_signatures(trx, available_keys); });
}

std::set<public_key_type>
database_api_impl::get_required_signatures(const signed_transaction& trx,
                                           const flat_set<public_key_type>& available_keys) const
{
    //   wdump((trx)(available_keys));
    auto result = trx.get_required_signatures(
        get_chain_id(), available_keys,
        [&](const std::string& account_name) {
            return authority(_db.get<account_authority_object, by_account>(account_name).active);
        },
        [&](const std::string& account_name) {
            return authority(_db.get<account_authority_object, by_account>(account_name).owner);
        },
        [&](const std::string& account_name) {
            return authority(_db.get<account_authority_object, by_account>(account_name).posting);
        },
        SCORUM_MAX_SIG_CHECK_DEPTH);
    //   wdump((result));
    return result;
}

std::set<public_key_type> database_api::get_potential_signatures(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->get_potential_signatures(trx); });
}

std::set<public_key_type> database_api_impl::get_potential_signatures(const signed_transaction& trx) const
{
    //   wdump((trx));
    std::set<public_key_type> result;
    trx.get_required_signatures(
        get_chain_id(), flat_set<public_key_type>(),
        [&](account_name_type account_name) {
            const auto& auth = _db.get<account_authority_object, by_account>(account_name).active;
            for (const auto& k : auth.get_keys())
                result.insert(k);
            return authority(auth);
        },
        [&](account_name_type account_name) {
            const auto& auth = _db.get<account_authority_object, by_account>(account_name).owner;
            for (const auto& k : auth.get_keys())
                result.insert(k);
            return authority(auth);
        },
        [&](account_name_type account_name) {
            const auto& auth = _db.get<account_authority_object, by_account>(account_name).posting;
            for (const auto& k : auth.get_keys())
                result.insert(k);
            return authority(auth);
        },
        SCORUM_MAX_SIG_CHECK_DEPTH);

    //   wdump((result));
    return result;
}

bool database_api::verify_authority(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->verify_authority(trx); });
}

bool database_api_impl::verify_authority(const signed_transaction& trx) const
{
    trx.verify_authority(get_chain_id(),
                         [&](const std::string& account_name) {
                             return authority(_db.get<account_authority_object, by_account>(account_name).active);
                         },
                         [&](const std::string& account_name) {
                             return authority(_db.get<account_authority_object, by_account>(account_name).owner);
                         },
                         [&](const std::string& account_name) {
                             return authority(_db.get<account_authority_object, by_account>(account_name).posting);
                         },
                         SCORUM_MAX_SIG_CHECK_DEPTH);
    return true;
}

bool database_api::verify_account_authority(const std::string& name_or_id,
                                            const flat_set<public_key_type>& signers) const
{
    return my->_db.with_read_lock([&]() { return my->verify_account_authority(name_or_id, signers); });
}

bool database_api_impl::verify_account_authority(const std::string& name, const flat_set<public_key_type>& keys) const
{
    FC_ASSERT(name.size() > 0);
    auto account = _db.find<account_object, by_name>(name);
    FC_ASSERT(account, "no such account");

    /// reuse trx.verify_authority by creating a dummy transfer
    signed_transaction trx;
    transfer_operation op;
    op.from = account->name;
    trx.operations.emplace_back(op);

    return verify_authority(trx);
}

std::vector<vote_state> database_api::get_active_votes(const std::string& author, const std::string& permlink) const
{
    return my->_db.with_read_lock([&]() {
        std::vector<vote_state> result;
        const auto& comment = my->_db.obtain_service<dbs_comment>().get(author, permlink);
        const auto& idx = my->_db.get_index<comment_vote_index>().indices().get<by_comment_voter>();
        comment_id_type cid(comment.id);
        auto itr = idx.lower_bound(cid);
        while (itr != idx.end() && itr->comment == cid)
        {
            const auto& vo = my->_db.get(itr->voter);
            vote_state vstate;
            vstate.voter = vo.name;
            vstate.weight = itr->weight;
            vstate.rshares = itr->rshares.value;
            vstate.percent = itr->vote_percent;
            vstate.time = itr->last_update;

            result.push_back(vstate);
            ++itr;
        }
        return result;
    });
}

std::vector<account_vote> database_api::get_account_votes(const std::string& voter) const
{
    return my->_db.with_read_lock([&]() {
        std::vector<account_vote> result;

        const auto& voter_acnt = my->_db.obtain_service<chain::dbs_account>().get_account(voter);
        const auto& idx = my->_db.get_index<comment_vote_index>().indices().get<by_voter_comment>();

        account_id_type aid(voter_acnt.id);
        auto itr = idx.lower_bound(aid);
        auto end = idx.upper_bound(aid);
        while (itr != end)
        {
            const auto& vo = my->_db.get(itr->comment);
            account_vote avote;
            avote.authorperm = vo.author + "/" + fc::to_string(vo.permlink);
            avote.weight = itr->weight;
            avote.rshares = itr->rshares.value;
            avote.percent = itr->vote_percent;
            avote.time = itr->last_update;
            result.push_back(avote);
            ++itr;
        }
        return result;
    });
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Budgets                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////
std::vector<budget_api_obj> database_api::get_budgets(const budget_type type, const std::set<std::string>& names) const
{
    return my->_db.with_read_lock([&]() {
        switch (type)
        {
        case budget_type::post:
            return my->get_budgets(my->_db.post_budget_service(), names);
        case budget_type::banner:
            return my->get_budgets(my->_db.banner_budget_service(), names);
        default:
            return std::vector<budget_api_obj>();
        }
    });
}

std::set<std::string>
database_api::lookup_budget_owners(const budget_type type, const std::string& lower_bound_name, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() {
        switch (type)
        {
        case budget_type::post:
            return my->lookup_budget_owners(my->_db.post_budget_service(), lower_bound_name, limit);
        case budget_type::banner:
            return my->lookup_budget_owners(my->_db.banner_budget_service(), lower_bound_name, limit);
        default:
            return std::set<std::string>();
        }
    });
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Atomic Swap                                                      //
//                                                                  //
//////////////////////////////////////////////////////////////////////
std::vector<atomicswap_contract_api_obj> database_api::get_atomicswap_contracts(const std::string& owner) const
{
    return my->_db.with_read_lock([&]() { return my->get_atomicswap_contracts(owner); });
}

std::vector<atomicswap_contract_api_obj> database_api_impl::get_atomicswap_contracts(const std::string& owner) const
{
    std::vector<atomicswap_contract_api_obj> results;

    chain::dbs_account& account_service = _db.obtain_service<chain::dbs_account>();
    const chain::account_object& owner_obj = account_service.get_account(owner);

    chain::dbs_atomicswap& atomicswap_service = _db.obtain_service<chain::dbs_atomicswap>();

    auto contracts = atomicswap_service.get_contracts(owner_obj);
    for (const chain::atomicswap_contract_object& contract : contracts)
    {
        results.push_back(atomicswap_contract_api_obj(contract));
    }

    return results;
}

atomicswap_contract_info_api_obj database_api::get_atomicswap_contract(const std::string& from,
                                                                       const std::string& to,
                                                                       const std::string& secret_hash) const
{
    return my->_db.with_read_lock([&]() { return my->get_atomicswap_contract(from, to, secret_hash); });
}

atomicswap_contract_info_api_obj database_api_impl::get_atomicswap_contract(const std::string& from,
                                                                            const std::string& to,
                                                                            const std::string& secret_hash) const
{
    atomicswap_contract_info_api_obj result;

    chain::dbs_account& account_service = _db.obtain_service<chain::dbs_account>();
    const chain::account_object& from_obj = account_service.get_account(from);
    const chain::account_object& to_obj = account_service.get_account(to);

    chain::dbs_atomicswap& atomicswap_service = _db.obtain_service<chain::dbs_atomicswap>();

    const chain::atomicswap_contract_object& contract = atomicswap_service.get_contract(from_obj, to_obj, secret_hash);

    return atomicswap_contract_info_api_obj(contract);
}

std::vector<account_name_type> database_api::get_active_witnesses() const
{
    return my->_db.with_read_lock([&]() {
        const auto& wso = my->_db.obtain_service<chain::dbs_witness_schedule>().get();
        size_t n = wso.current_shuffled_witnesses.size();
        std::vector<account_name_type> result;
        result.reserve(n);
        for (size_t i = 0; i < n; i++)
            result.push_back(wso.current_shuffled_witnesses[i]);
        return result;
    });
}

std::vector<scorumpower_delegation_api_obj>
database_api::get_scorumpower_delegations(const std::string& account, const std::string& from, uint32_t limit) const
{
    FC_ASSERT(limit <= get_api_config(API_DATABASE).lookup_limit);

    return my->_db.with_read_lock([&]() {
        std::vector<scorumpower_delegation_api_obj> result;
        result.reserve(limit);

        const auto& delegation_idx = my->_db.get_index<scorumpower_delegation_index, by_delegation>();
        auto itr = delegation_idx.lower_bound(boost::make_tuple(account, from));
        while (result.size() < limit && itr != delegation_idx.end() && itr->delegator == account)
        {
            result.push_back(*itr);
            ++itr;
        }

        return result;
    });
}

std::vector<scorumpower_delegation_expiration_api_obj> database_api::get_expiring_scorumpower_delegations(
    const std::string& account, time_point_sec from, uint32_t limit) const
{
    FC_ASSERT(limit <= get_api_config(API_DATABASE).lookup_limit);

    return my->_db.with_read_lock([&]() {
        std::vector<scorumpower_delegation_expiration_api_obj> result;
        result.reserve(limit);

        const auto& exp_idx = my->_db.get_index<scorumpower_delegation_expiration_index, by_account_expiration>();
        auto itr = exp_idx.lower_bound(boost::make_tuple(account, from));
        while (result.size() < limit && itr != exp_idx.end() && itr->delegator == account)
        {
            result.push_back(*itr);
            ++itr;
        }

        return result;
    });
}

} // namespace app
} // namespace scorum
