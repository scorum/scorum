#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/account_object.hpp>

namespace scorum {
namespace chain {

dbs_account::dbs_account(database& db)
    : _BaseClass(db)
{
}

void dbs_account::create_account_by_faucets(const account_name_type& new_account_name,
                                            const account_name_type& creator_name,
                                            const public_key_type& memo_key,
                                            const string& json_metadata,
                                            const authority& owner,
                                            const authority& active,
                                            const authority& posting,
                                            const asset& fee)
{
    const auto& props = db_impl().get_dynamic_global_properties();
    const auto& creator = db_impl().get_account(creator_name);

    db_impl().modify(creator, [&](account_object& c) { c.balance -= fee; });

    const auto& new_account = db_impl().create<account_object>([&](account_object& acc) {
        acc.name = new_account_name;
        acc.memo_key = memo_key;
        acc.created = props.time;
        acc.last_vote_time = props.time;
        acc.mined = false;

        acc.recovery_account = creator_name;

#ifndef IS_LOW_MEM
        from_string(acc.json_metadata, json_metadata);
#endif
    });

    db_impl().create<account_authority_object>([&](account_authority_object& auth) {
        auth.account = new_account_name;
        auth.owner = owner;
        auth.active = active;
        auth.posting = posting;
        auth.last_owner_update = fc::time_point_sec::min();
    });

    if (fee.amount > 0)
        db_impl().create_vesting(new_account, fee);
}

void dbs_account::create_account_with_delegation(const account_name_type& new_account_name,
                                                 const account_name_type& creator_name,
                                                 const public_key_type& memo_key,
                                                 const string& json_metadata,
                                                 const authority& owner,
                                                 const authority& active,
                                                 const authority& posting,
                                                 const asset& fee,
                                                 const asset& delegation)
{
    const auto& props = db_impl().get_dynamic_global_properties();
    const auto& creator = db_impl().get_account(creator_name);

    db_impl().modify(creator, [&](account_object& c) {
        c.balance -= fee;
        c.delegated_vesting_shares += delegation;
    });

    const auto& new_account = db_impl().create<account_object>([&](account_object& acc) {
        acc.name = new_account_name;
        acc.memo_key = memo_key;
        acc.created = props.time;
        acc.last_vote_time = props.time;
        acc.mined = false;

        acc.recovery_account = creator_name;

        acc.received_vesting_shares = delegation;

#ifndef IS_LOW_MEM
        from_string(acc.json_metadata, json_metadata);
#endif
    });

    db_impl().create<account_authority_object>([&](account_authority_object& auth) {
        auth.account = new_account_name;
        auth.owner = owner;
        auth.active = active;
        auth.posting = posting;
        auth.last_owner_update = fc::time_point_sec::min();
    });

    if (delegation.amount > 0)
    {
        db_impl().create<vesting_delegation_object>([&](vesting_delegation_object& vdo) {
            vdo.delegator = creator_name;
            vdo.delegatee = new_account_name;
            vdo.vesting_shares = delegation;
            vdo.min_delegation_time = db_impl().head_block_time() + SCORUM_CREATE_ACCOUNT_DELEGATION_TIME;
        });
    }

    if (fee.amount > 0)
        db_impl().create_vesting(new_account, fee);
}

const account_object& dbs_account::get_account(const account_name_type& name) const
{
    try
    {
        return db_impl().get<account_object, by_name>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

const account_authority_object& dbs_account::get_account_authority(const account_name_type& name) const
{
    try
    {
        return db_impl().get<account_authority_object, by_account>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

void dbs_account::update_acount(const account_object& account,
    const account_authority_object& account_authority,
                   const public_key_type& memo_key,
                   const string& json_metadata,
                    const optional<authority> &owner,
                    const optional<authority> &active,
                    const optional<authority> &posting)
{
    db_impl().modify(account, [&](account_object& acc) {
        if (memo_key != public_key_type())
            acc.memo_key = memo_key;

        if ((active || owner) && acc.active_challenged)
        {
            acc.active_challenged = false;
            acc.last_active_proved = db_impl().head_block_time();
        }

        acc.last_account_update = db_impl().head_block_time();

#ifndef IS_LOW_MEM
        if (json_metadata.size() > 0)
            from_string(acc.json_metadata, json_metadata);
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
    if (db_impl().head_block_num() >= SCORUM_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM)
    {
        db_impl().create<owner_authority_history_object>([&](owner_authority_history_object& hist) {
            hist.account = account.name;
            hist.previous_owner_authority = db_impl().get<account_authority_object, by_account>(account.name).owner;
            hist.last_valid_time = db_impl().head_block_time();
        });
    }

    db_impl().modify(db_impl().get<account_authority_object, by_account>(account.name),
                     [&](account_authority_object& auth) {
                         auth.owner = owner_authority;
                         auth.last_owner_update = db_impl().head_block_time();
                     });
}

void dbs_account::check_account_existence(const account_name_type& name) const
{
    get_account(name);
}

void dbs_account::check_account_existence(const account_authority_map& names) const
{
    for (const auto& a : names)
    {
        check_account_existence(a.first);
    }
}

void dbs_account::prove_authority(const account_object& challenged, bool require_owner)
{
    db_impl().modify(challenged, [&](account_object& a) {
        a.active_challenged = false;
        a.last_active_proved = db_impl().head_block_time();
        if (require_owner)
        {
            a.owner_challenged = false;
            a.last_owner_proved = db_impl().head_block_time();
        }
    });
}
}
}
