#include <boost/test/unit_test.hpp>

#include <scorum/app/api_context.hpp>

#include <scorum/blockchain_statistics/blockchain_statistics_plugin.hpp>
#include <scorum/blockchain_statistics/blockchain_statistics_api.hpp>
#include <scorum/common_statistics/base_plugin_impl.hpp>
#include <scorum/chain/services/witness_schedule.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/schema/account_objects.hpp>

#include "database_trx_integration.hpp"

using namespace scorum;
using namespace scorum::blockchain_statistics;
using namespace scorum::app;
using namespace database_fixture;

namespace blockchain_stat {

struct stat_database_fixture : public database_trx_integration_fixture
{
    Actor alice;
    Actor bob;

    Actor witness1;
    Actor witness2;

    api_context _api_ctx;
    blockchain_statistics::blockchain_statistics_api _api_call;

    stat_database_fixture()
        : alice("alice")
        , bob("bob")
        , witness1("witness1")
        , witness2("witness2")
        , _api_ctx(app, API_BLOCKCHAIN_STATISTICS, std::make_shared<api_session_data>())
        , _api_call(_api_ctx)
    {
        witness1.public_key = initdelegate.public_key;
        witness2.public_key = initdelegate.public_key;

        static const asset registration_bonus = ASSET_SCR(100);
        genesis_state_type::registration_schedule_item single_stage{ 1u, 1u, 100u };
        genesis_state_type genesis = database_integration_fixture::default_genesis_state()
                                         .registration_supply(registration_bonus * 100)
                                         .registration_bonus(registration_bonus)
                                         .registration_schedule(single_stage)
                                         .committee(TEST_INIT_DELEGATE_NAME)
                                         .accounts(witness1, witness2)
                                         .witnesses(witness1, witness2)
                                         .generate();

        open_database(genesis);

        init_plugin<scorum::blockchain_statistics::blockchain_statistics_plugin>();

        generate_block();

        vest(initdelegate, 10000);

        account_create(alice, initdelegate.public_key);
        fund(alice, SCORUM_MIN_PRODUCER_REWARD);
        vest(alice, SCORUM_MIN_PRODUCER_REWARD);

        account_create(bob, initdelegate.public_key);
        vest(bob, SCORUM_MIN_PRODUCER_REWARD);

        generate_block();
        validate_database();
    }

    const bucket_object& get_lifetime_bucket() const
    {
        const auto& bucket_idx = db.get_index<bucket_index>().indices().get<common_statistics::by_bucket>();
        auto itr = bucket_idx.find(boost::make_tuple(LIFE_TIME_PERIOD, fc::time_point_sec()));
        FC_ASSERT(itr != bucket_idx.end());
        return *itr;
    }

    void start_withdraw(share_type to_withdraw)
    {
        withdraw_scorumpower_operation op;
        op.account = bob;
        op.scorumpower = asset(to_withdraw, SP_SYMBOL);

        push_operation(op);
    }
};
} // namespace blockchain_stat

BOOST_FIXTURE_TEST_SUITE(statistic_tests, blockchain_stat::stat_database_fixture)

SCORUM_TEST_CASE(get_missed_blocks_via_api_test)
{
    statistics stat = _api_call.get_lifetime_stats();

    BOOST_REQUIRE(stat.missed_blocks.empty());

    db_plugin->debug_update(
        [=](database& db) {
            db.obtain_service<dbs_witness_schedule>().update(
                [&](witness_schedule_object& wso) { wso.num_scheduled_witnesses = 3; });
        },
        default_skip);

    auto current_block = db.head_block_num();

    auto slots_to_miss = 1u;
    db_plugin->debug_generate_blocks(debug_key, 1, default_skip, slots_to_miss);

    stat = _api_call.get_lifetime_stats();

    BOOST_REQUIRE_EQUAL(stat.missed_blocks.size(), 1u);
    BOOST_REQUIRE_EQUAL(stat.missed_blocks.begin()->first, current_block + 1);
    BOOST_REQUIRE_EQUAL(stat.missed_blocks.begin()->second, initdelegate.name);
}

SCORUM_TEST_CASE(produced_blocks_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.blocks;

    generate_block();

    BOOST_REQUIRE_EQUAL(bucket.blocks, orig_val + 1);
}

SCORUM_TEST_CASE(transactions_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.transactions;

    transfer_operation op;
    op.from = TEST_INIT_DELEGATE_NAME;
    op.to = alice;
    op.amount = asset(1, SCORUM_SYMBOL);

    push_operation(op);

    BOOST_REQUIRE_EQUAL(bucket.transactions, orig_val + 1);
}

SCORUM_TEST_CASE(operations_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.operations;

    transfer_operation op;
    op.from = TEST_INIT_DELEGATE_NAME;
    op.to = alice;
    op.amount = asset(1, SCORUM_SYMBOL);

    push_operation(op);

    BOOST_REQUIRE_EQUAL(bucket.operations, orig_val + 1);
}

SCORUM_TEST_CASE(bandwidth_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.bandwidth;

    transfer_operation op;
    op.from = TEST_INIT_DELEGATE_NAME;
    op.to = alice;
    op.amount = asset(1, SCORUM_SYMBOL);

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.validate();
    db.push_transaction(tx, default_skip);

    generate_block();

    BOOST_REQUIRE_EQUAL(bucket.bandwidth, orig_val + fc::raw::pack_size(tx));
}

SCORUM_TEST_CASE(account_creation_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.paid_accounts_created;

    account_create_operation op;
    op.fee = SUFFICIENT_FEE;
    op.creator = TEST_INIT_DELEGATE_NAME;
    op.new_account_name = "maugli";

    push_operation(op);

    BOOST_REQUIRE_EQUAL(bucket.paid_accounts_created, orig_val + 1);
}

SCORUM_TEST_CASE(account_creation_with_delegation_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.paid_accounts_created;

    account_create_with_delegation_operation op;
    op.fee = SUFFICIENT_FEE;
    op.creator = "alice";
    op.new_account_name = "maugli";

    push_operation(op);

    BOOST_REQUIRE_EQUAL(bucket.paid_accounts_created, orig_val + 1);
}

SCORUM_TEST_CASE(account_creation_by_committee_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.free_accounts_created;

    account_create_by_committee_operation op;
    op.creator = TEST_INIT_DELEGATE_NAME;
    op.new_account_name = "maugli";

    push_operation(op);

    BOOST_REQUIRE_EQUAL(bucket.free_accounts_created, orig_val + 1);
}

SCORUM_TEST_CASE(transfers_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.transfers;
    auto orig_val_scr = bucket.scorum_transferred;

    transfer_operation op;
    op.from = TEST_INIT_DELEGATE_NAME;
    op.to = alice;
    op.amount = asset(1, SCORUM_SYMBOL);

    push_operation(op);

    BOOST_REQUIRE_EQUAL(bucket.transfers, orig_val + 1);
    BOOST_REQUIRE_EQUAL(bucket.scorum_transferred, orig_val_scr + 1);
}

SCORUM_TEST_CASE(transfers_to_scorumpower_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.transfers_to_scorumpower;
    auto orig_val_scr = bucket.scorum_transferred_to_scorumpower;

    transfer_to_scorumpower_operation op;
    op.from = TEST_INIT_DELEGATE_NAME;
    op.to = alice;
    op.amount = asset(1, SCORUM_SYMBOL);

    push_operation(op);

    BOOST_REQUIRE_EQUAL(bucket.transfers_to_scorumpower, orig_val + 1);
    BOOST_REQUIRE_EQUAL(bucket.scorum_transferred_to_scorumpower, orig_val_scr + 1);
}

SCORUM_TEST_CASE(vesting_withdrawals_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    const account_object& alice_acc = db.obtain_service<dbs_account>().get_account(alice.name);

    {
        auto orig_val = bucket.new_vesting_withdrawal_requests;

        withdraw_scorumpower_operation op;
        op.account = alice;
        op.scorumpower = alice_acc.scorumpower;

        push_operation(op);

        BOOST_REQUIRE_EQUAL(bucket.new_vesting_withdrawal_requests, orig_val + 1);
    }
    {
        auto orig_val = bucket.modified_vesting_withdrawal_requests;

        withdraw_scorumpower_operation op;
        op.account = alice;
        op.scorumpower = alice_acc.scorumpower / 2;

        push_operation(op);

        BOOST_REQUIRE_EQUAL(bucket.modified_vesting_withdrawal_requests, orig_val + 1);
    }
}

SCORUM_TEST_CASE(vesting_withdrawals_finish_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.finished_vesting_withdrawals;
    auto orig_val_processed = bucket.vesting_withdrawals_processed;
    auto orig_val_rate = bucket.vesting_withdraw_rate_delta;

    start_withdraw(share_type(SCORUM_VESTING_WITHDRAW_INTERVALS));

    for (uint32_t i = 0; i < SCORUM_VESTING_WITHDRAW_INTERVALS; ++i)
    {
        BOOST_REQUIRE_EQUAL(bucket.finished_vesting_withdrawals, orig_val);
        BOOST_REQUIRE_EQUAL(bucket.vesting_withdrawals_processed, orig_val_processed + i);
        BOOST_REQUIRE_EQUAL(bucket.vesting_withdraw_rate_delta, orig_val_rate + 1);

        fc::time_point_sec advance = db.head_block_time() + fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);

        generate_blocks(advance, true);
        generate_block(); // call full apply_block procedure
    }

    BOOST_REQUIRE_EQUAL(bucket.finished_vesting_withdrawals, orig_val + 1);
    BOOST_REQUIRE_EQUAL(bucket.vesting_withdrawals_processed, orig_val_processed + SCORUM_VESTING_WITHDRAW_INTERVALS);
    BOOST_REQUIRE_EQUAL(bucket.vesting_withdraw_rate_delta, orig_val_rate);
}

SCORUM_TEST_CASE(vesting_withdrawn_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.scorumpower_withdrawn;

    start_withdraw(share_type(SCORUM_VESTING_WITHDRAW_INTERVALS));

    fc::time_point_sec advance = db.head_block_time() + fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
    generate_blocks(advance, true);
    generate_block(); // call full apply_block procedure

    BOOST_REQUIRE_EQUAL(bucket.scorumpower_withdrawn, orig_val + 1);
}

SCORUM_TEST_CASE(scorumpower_transfered_stat_test)
{
    const bucket_object& bucket = get_lifetime_bucket();

    auto orig_val = bucket.scorumpower_transferred;

    start_withdraw(share_type(SCORUM_VESTING_WITHDRAW_INTERVALS));

    set_withdraw_scorumpower_route_to_account_operation op;
    op.from_account = "bob";
    op.to_account = alice;
    op.auto_vest = true;
    op.percent = SCORUM_PERCENT(100);

    push_operation(op);

    fc::time_point_sec advance = db.head_block_time() + fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
    generate_blocks(advance, true);
    generate_block(); // call full apply_block procedure

    BOOST_REQUIRE_EQUAL(bucket.scorumpower_transferred, orig_val + 1);
}

BOOST_AUTO_TEST_SUITE_END()
