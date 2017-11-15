#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/account_object.hpp>

namespace scorum {
namespace chain {

dbs_account::dbs_account(dbservice &db): _db(static_cast<database &>(db))
{
}

void dbs_account::write_account_creation_by_faucets(
                       const account_name_type& new_account_name,
                       const account_name_type& creator_name,
                       const public_key_type &memo_key,
                       const string &json_metadata,
                       const authority& owner,
                       const authority& active,
                       const authority& posting,
                       const asset &fee)
{
    const auto& props = _db.get_dynamic_global_properties();
    const auto& creator = _db.get_account(creator_name);

    _db.modify(creator, [&](account_object& c) { c.balance -= fee; });

    const auto& new_account = _db.create<account_object>([&](account_object& acc) {
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

    _db.create<account_authority_object>([&](account_authority_object& auth) {
        auth.account = new_account_name;
        auth.owner = owner;
        auth.active = active;
        auth.posting = posting;
        auth.last_owner_update = fc::time_point_sec::min();
    });

    if (fee.amount > 0)
        _db.create_vesting(new_account, fee);
}

void dbs_account::write_account_creation_with_delegation(
                       const account_name_type& new_account_name,
                       const account_name_type& creator_name,
                       const public_key_type &memo_key,
                       const string &json_metadata,
                       const authority& owner,
                       const authority& active,
                       const authority& posting,
                       const asset &fee,
                       const asset &delegation)
{
    const auto& props = _db.get_dynamic_global_properties();
    const auto& creator = _db.get_account(creator_name);

    _db.modify(creator, [&](account_object& c) {
        c.balance -= fee;
        c.delegated_vesting_shares += delegation;
    });

    const auto& new_account = _db.create<account_object>([&](account_object& acc) {
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

    _db.create<account_authority_object>([&](account_authority_object& auth) {
        auth.account = new_account_name;
        auth.owner = owner;
        auth.active = active;
        auth.posting = posting;
        auth.last_owner_update = fc::time_point_sec::min();
    });

    if (delegation.amount > 0)
    {
        _db.create<vesting_delegation_object>([&](vesting_delegation_object& vdo) {
            vdo.delegator = creator_name;
            vdo.delegatee = new_account_name;
            vdo.vesting_shares = delegation;
            vdo.min_delegation_time = _db.head_block_time() + SCORUM_CREATE_ACCOUNT_DELEGATION_TIME;
        });
    }

    if (fee.amount > 0)
        _db.create_vesting(new_account, fee);
}

}
}
