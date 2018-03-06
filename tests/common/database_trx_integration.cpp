#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/operation_object.hpp>
#include <scorum/account_history/account_history_plugin.hpp>
#include <scorum/witness/witness_plugin.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/account_history/schema/account_history_object.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_trx_integration.hpp"

namespace scorum {
namespace chain {

database_trx_integration_fixture::database_trx_integration_fixture()
{
}

database_trx_integration_fixture::~database_trx_integration_fixture()
{
}

const account_object& database_trx_integration_fixture::account_create(const std::string& name,
                                                                       const std::string& creator,
                                                                       const private_key_type& creator_key,
                                                                       const share_type& fee,
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

        const account_object& acct = db.obtain_service<chain::dbs_account>().get_account(name);

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
        return account_create(name, TEST_INIT_DELEGATE_NAME, init_account_priv_key,
                              std::max(db.get_dynamic_global_properties().median_chain_props.account_creation_fee.amount
                                           * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER,
                                       (SUFFICIENT_FEE).amount),
                              key, post_key, "");
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

        return db.get_witness(owner);
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
        db.push_transaction(trx, default_skip);
        trx.operations.clear();
    }
    FC_CAPTURE_AND_RETHROW((from)(to)(amount))
}

void database_trx_integration_fixture::transfer_to_vest(const std::string& from,
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
        db.push_transaction(trx, default_skip);
        trx.operations.clear();
    }
    FC_CAPTURE_AND_RETHROW((from)(to)(amount))
}

void database_trx_integration_fixture::transfer_to_vest(const std::string& from,
                                                        const std::string& to,
                                                        const share_type& amount)
{
    transfer_to_vest(from, to, asset(amount, SCORUM_SYMBOL));
}

void database_trx_integration_fixture::vest(const std::string& account_name, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SCORUM_SYMBOL, "Invalid asset type (symbol) in ${1}.", ("1", __FUNCTION__));

    try
    {
        transfer_to_vest(TEST_INIT_DELEGATE_NAME, account_name, amount);
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
    return db.obtain_service<chain::dbs_account>().get_account(account_name).balance;
}

void database_trx_integration_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
{
    trx.sign(key, db.get_chain_id());
}

std::vector<operation> database_trx_integration_fixture::get_last_operations(uint32_t num_ops)
{
    std::vector<operation> ops;
    const auto& acc_hist_idx
        = db.get_index<account_history::account_operations_full_history_index>().indices().get<by_id>();
    auto itr = acc_hist_idx.end();

    while (itr != acc_hist_idx.begin() && ops.size() < num_ops)
    {
        itr--;
        ops.push_back(fc::raw::unpack<scorum::chain::operation>(db.get(itr->op).serialized_op));
    }

    return ops;
}

void database_trx_integration_fixture::open_database_impl(const genesis_state_type& genesis)
{
    database_integration_fixture::open_database_impl(genesis);

    generate_block();
    db.set_hardfork(SCORUM_NUM_HARDFORKS);
    generate_block();

    if (db.find<witness_object, by_name>(TEST_INIT_DELEGATE_NAME))
    {
        generate_blocks(2);

        vest(TEST_INIT_DELEGATE_NAME, 10000);

        // Fill up the rest of the required miners
        for (int i = SCORUM_NUM_INIT_DELEGATES; i < SCORUM_MAX_WITNESSES; i++)
        {
            account_create(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_pub_key);
            fund(TEST_INIT_DELEGATE_NAME + fc::to_string(i), SCORUM_MIN_PRODUCER_REWARD);
            witness_create(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_priv_key, "foo.bar",
                           init_account_pub_key, SCORUM_MIN_PRODUCER_REWARD.amount);
        }

        generate_blocks(2);
    }
}

} // namespace chain
} // namespace scorum
