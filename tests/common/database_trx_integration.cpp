#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/witness/witness_plugin.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_trx_integration.hpp"

namespace database_fixture {

database_trx_integration_fixture::database_trx_integration_fixture()
    : services(db)
{
}

database_trx_integration_fixture::~database_trx_integration_fixture()
{
}

ActorActions database_trx_integration_fixture::actor(const Actor& a)
{
    ActorActions c(*this, a);
    return c;
}

share_type database_trx_integration_fixture::get_account_creation_fee() const
{
    const share_type current_account_creation_fee
        = db.obtain_service<dbs_dynamic_global_property>().get().median_chain_props.account_creation_fee.amount
        * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER;

    return std::max(current_account_creation_fee, (SUFFICIENT_FEE).amount);
}

const account_object& database_trx_integration_fixture::account_create(const std::string& name,
                                                                       const std::string& creator,
                                                                       const private_key_type& creator_key,
                                                                       share_type fee,
                                                                       const public_key_type& key,
                                                                       const public_key_type& post_key,
                                                                       const std::string& json_metadata)
{
    try
    {
        account_create_with_delegation_operation op;
        op.new_account_name = name;
        op.creator = creator;
        op.fee = asset(fee, SCORUM_SYMBOL);
        op.delegation = asset(0, SP_SYMBOL);
        op.owner = authority(1, key, 1);
        op.active = authority(1, key, 1);
        op.posting = authority(1, post_key, 1);
        op.memo_key = key;
        op.json_metadata = json_metadata;

        trx.operations.push_back(op);

        trx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(creator_key, db.get_chain_id());
        trx.validate();
        db.push_transaction(trx, 0);
        trx.operations.clear();
        trx.signatures.clear();

        const account_object& acct = db.obtain_service<dbs_account>().get_account(name);

        return acct;
    }
    FC_CAPTURE_AND_RETHROW((name)(creator))
}

const account_object& database_trx_integration_fixture::account_create(const std::string& name,
                                                                       const public_key_type& key,
                                                                       const public_key_type& post_key)
{
    try
    {
        // clang-format off
        return account_create(name,
                              initdelegate.name,
                              initdelegate.private_key,
                              get_account_creation_fee(),
                              key,
                              post_key,
                              "");
        // clang-format on
    }
    FC_CAPTURE_AND_RETHROW((name));
}

const account_object& database_trx_integration_fixture::account_create(const std::string& name,
                                                                       const public_key_type& key)
{
    return account_create(name, key, key);
}

const witness_object& database_trx_integration_fixture::witness_create(const std::string& owner,
                                                                       const private_key_type& owner_key,
                                                                       const std::string& url,
                                                                       const public_key_type& signing_key,
                                                                       const share_type& fee)
{
    try
    {
        witness_update_operation op;
        op.owner = owner;
        op.url = url;
        op.block_signing_key = signing_key;

        trx.operations.push_back(op);
        trx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(owner_key, db.get_chain_id());
        trx.validate();
        db.push_transaction(trx, 0);
        trx.operations.clear();
        trx.signatures.clear();

        return db.obtain_service<dbs_witness>().get(owner);
    }
    FC_CAPTURE_AND_RETHROW((owner)(url))
}

void database_trx_integration_fixture::fund(const std::string& account_name, const share_type& amount)
{
    try
    {
        transfer(TEST_INIT_DELEGATE_NAME, account_name, asset(amount, SCORUM_SYMBOL));
    }
    FC_CAPTURE_AND_RETHROW((account_name)(amount))
}

void database_trx_integration_fixture::fund(const std::string& account_name, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SCORUM_SYMBOL, "Invalid asset type (symbol) in ${1}.", ("1", __FUNCTION__));

    try
    {
        transfer(TEST_INIT_DELEGATE_NAME, account_name, amount);
    }
    FC_CAPTURE_AND_RETHROW((account_name)(amount))
}

void database_trx_integration_fixture::transfer(const std::string& from, const std::string& to, const asset& amount)
{
    try
    {
        transfer_operation op;
        op.from = from;
        op.to = to;
        op.amount = amount;

        trx.operations.push_back(op);
        trx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.validate();
        db.push_transaction(trx, get_skip_flags());
        trx.operations.clear();
    }
    FC_CAPTURE_AND_RETHROW((from)(to)(amount))
}

void database_trx_integration_fixture::transfer_to_scorumpower(const std::string& from,
                                                               const std::string& to,
                                                               const asset& amount)
{
    try
    {
        transfer_to_scorumpower_operation op;
        op.from = from;
        op.to = to;
        op.amount = amount;

        trx.operations.push_back(op);
        trx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.validate();
        db.push_transaction(trx, get_skip_flags());
        trx.operations.clear();
    }
    FC_CAPTURE_AND_RETHROW((from)(to)(amount))
}

void database_trx_integration_fixture::transfer_to_scorumpower(const std::string& from,
                                                               const std::string& to,
                                                               const share_type& amount)
{
    transfer_to_scorumpower(from, to, asset(amount, SCORUM_SYMBOL));
}

void database_trx_integration_fixture::vest(const std::string& account_name, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SCORUM_SYMBOL, "Invalid asset type (symbol) in ${1}.", ("1", __FUNCTION__));

    try
    {
        transfer_to_scorumpower(TEST_INIT_DELEGATE_NAME, account_name, amount);
    }
    FC_CAPTURE_AND_RETHROW((account_name)(amount))
}

void database_trx_integration_fixture::vest(const std::string& from, const share_type& amount)
{
    vest(from, asset(amount, SCORUM_SYMBOL));
}

void database_trx_integration_fixture::proxy(const std::string& account, const std::string& proxy)
{
    try
    {
        account_witness_proxy_operation op;
        op.account = account;
        op.proxy = proxy;
        trx.operations.push_back(op);
        db.push_transaction(trx, ~0);
        trx.operations.clear();
    }
    FC_CAPTURE_AND_RETHROW((account)(proxy))
}

const asset& database_trx_integration_fixture::get_balance(const std::string& account_name) const
{
    return db.obtain_service<dbs_account>().get_account(account_name).balance;
}

void database_trx_integration_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
{
    trx.sign(key, db.get_chain_id());
}

void database_trx_integration_fixture::open_database_impl(const genesis_state_type& genesis)
{
    database_integration_fixture::open_database_impl(genesis);

    generate_block();
    db.set_hardfork(SCORUM_NUM_HARDFORKS);
    generate_block();

    if (db.find<witness_object, by_name>(initdelegate.name))
    {
        generate_blocks(2);

        vest(initdelegate.name, 10000);

        // Fill up the rest of the required miners
        for (int i = TEST_NUM_INIT_DELEGATES; i < SCORUM_MAX_WITNESSES; i++)
        {
            account_create(initdelegate.name + fc::to_string(i), initdelegate.public_key);
            fund(initdelegate.name + fc::to_string(i), SCORUM_MIN_PRODUCER_REWARD);
            witness_create(initdelegate.name + fc::to_string(i), initdelegate.private_key, "foo.bar",
                           initdelegate.public_key, SCORUM_MIN_PRODUCER_REWARD.amount);
        }

        generate_blocks(2);
    }
}

} // database_fixture
