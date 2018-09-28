#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/app/api_context.hpp>

#include <scorum/blockchain_history/blockchain_history_plugin.hpp>
#include <scorum/blockchain_history/schema/history_object.hpp>
#include <scorum/blockchain_history/schema/applied_operation.hpp>

#include <scorum/blockchain_history/account_history_api.hpp>
#include <scorum/blockchain_history/blockchain_history_api.hpp>
#include <scorum/blockchain_history/devcommittee_history_api.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/common_api/config_api.hpp>

#include "database_trx_integration.hpp"
#include "devcommittee_fixture.hpp"
#include "operation_check.hpp"

namespace blockchain_history_tests {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;
using fc::string;

using namespace operation_tests;

struct history_database_fixture : public database_fixture::database_trx_integration_fixture
{
    using operation_map_type = std::map<uint32_t, blockchain_history::applied_operation>;
    using saved_operation_vector_type = std::vector<operation_map_type::value_type>;

    history_database_fixture()
        : buratino("buratino")
        , maugli("maugli")
        , alice("alice")
        , bob("bob")
        , sam("sam")
        , _account_history_api_ctx(app, API_ACCOUNT_HISTORY, std::make_shared<api_session_data>())
        , _api(_account_history_api_ctx)
    {
        init_plugin<scorum::blockchain_history::blockchain_history_plugin>();

        open_database();
        generate_block();
        validate_database();

        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, feed_amount);
        actor(initdelegate).give_sp(alice, feed_amount);

        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_scr(bob, feed_amount);
        actor(initdelegate).give_sp(bob, feed_amount);

        actor(initdelegate).create_account(sam);
    }

    template <typename history_object_type>
    operation_map_type get_operations_accomplished_by_account(const std::string& account_name)
    {
        const auto& idx = db.get_index<blockchain_history::account_history_index<history_object_type>>()
                              .indices()
                              .get<blockchain_history::by_account>();

        auto itr = idx.lower_bound(boost::make_tuple(account_name, uint32_t(-1)));
        if (itr != idx.end())
            idump((*itr));
        auto end = idx.upper_bound(boost::make_tuple(account_name, int64_t(0)));
        if (end != idx.end())
            idump((*end));

        operation_map_type result;
        while (itr != end)
        {
            result[itr->sequence] = db.get(itr->op);
            ++itr;
        }
        return result;
    }

    const int feed_amount = 99000;

    Actor buratino;
    Actor maugli;
    Actor alice;
    Actor bob;
    Actor sam;

    api_context _account_history_api_ctx;
    blockchain_history::account_history_api _api;
};

BOOST_FIXTURE_TEST_SUITE(account_history_tests, blockchain_history_tests::history_database_fixture)

SCORUM_TEST_CASE(check_account_nontransfer_operation_only_in_full_history_test)
{
    actor(initdelegate).create_account(buratino);

    operation_map_type buratino_full_ops
        = get_operations_accomplished_by_account<blockchain_history::account_history_object>(buratino);
    operation_map_type buratino_scr_ops
        = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_scr_history_object>(buratino);
    operation_map_type buratino_sp_ops
        = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_sp_history_object>(buratino);

    BOOST_REQUIRE_EQUAL(buratino_full_ops.size(), 1u);
    BOOST_REQUIRE_EQUAL(buratino_scr_ops.size(), 0u);
    BOOST_REQUIRE_EQUAL(buratino_sp_ops.size(), 0u);

    BOOST_REQUIRE(buratino_full_ops[0].op.which() == operation::tag<account_create_with_delegation_operation>::value);
}

SCORUM_TEST_CASE(check_account_transfer_operation_in_full_and_transfers_to_scr_history_test)
{
    actor(initdelegate).create_account(buratino);
    actor(initdelegate).give_scr(buratino, SCORUM_MIN_PRODUCER_REWARD.amount.value);

    operation_map_type buratino_full_ops
        = get_operations_accomplished_by_account<blockchain_history::account_history_object>(buratino);
    operation_map_type buratino_scr_ops
        = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_scr_history_object>(buratino);
    operation_map_type buratino_sp_ops
        = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_sp_history_object>(buratino);

    BOOST_REQUIRE_EQUAL(buratino_full_ops.size(), 2u);
    BOOST_REQUIRE_EQUAL(buratino_scr_ops.size(), 1u);
    BOOST_REQUIRE_EQUAL(buratino_sp_ops.size(), 0u);

    BOOST_REQUIRE(buratino_full_ops[0].op.which() == operation::tag<account_create_with_delegation_operation>::value);
    BOOST_REQUIRE(buratino_full_ops[1].op.which() == operation::tag<transfer_operation>::value);
    BOOST_REQUIRE(buratino_scr_ops[0].op.which() == operation::tag<transfer_operation>::value);

    BOOST_REQUIRE(buratino_scr_ops[0].op == buratino_full_ops[1].op);
}

SCORUM_TEST_CASE(check_account_transfer_operation_in_full_and_transfers_to_sp_history_test)
{
    actor(initdelegate).create_account(buratino);
    actor(initdelegate).give_sp(buratino, SCORUM_MIN_PRODUCER_REWARD.amount.value);

    operation_map_type buratino_full_ops
        = get_operations_accomplished_by_account<blockchain_history::account_history_object>(buratino);
    operation_map_type buratino_scr_ops
        = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_scr_history_object>(buratino);
    operation_map_type buratino_sp_ops
        = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_sp_history_object>(buratino);

    BOOST_REQUIRE_EQUAL(buratino_full_ops.size(), 2u);
    BOOST_REQUIRE_EQUAL(buratino_scr_ops.size(), 0u);
    BOOST_REQUIRE_EQUAL(buratino_sp_ops.size(), 1u);

    BOOST_REQUIRE(buratino_full_ops[0].op.which() == operation::tag<account_create_with_delegation_operation>::value);
    BOOST_REQUIRE(buratino_full_ops[1].op.which() == operation::tag<transfer_to_scorumpower_operation>::value);
    BOOST_REQUIRE(buratino_sp_ops[0].op.which() == operation::tag<transfer_to_scorumpower_operation>::value);

    BOOST_REQUIRE(buratino_sp_ops[0].op == buratino_full_ops[1].op);
}

SCORUM_TEST_CASE(check_account_transfer_operation_history_test)
{
    actor(initdelegate).create_account(buratino);
    actor(initdelegate).create_account(maugli);

    actor(initdelegate).give_scr(buratino, SCORUM_MIN_PRODUCER_REWARD.amount.value);

    {
        operation_map_type buratino_ops
            = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_scr_history_object>(
                buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_scr_history_object>(
                maugli);

        BOOST_REQUIRE_EQUAL(buratino_ops.size(), 1u);
        BOOST_REQUIRE_EQUAL(maugli_ops.size(), 0u);

        transfer_operation& op = buratino_ops[0].op.get<transfer_operation>();
        BOOST_REQUIRE_EQUAL(op.from, TEST_INIT_DELEGATE_NAME);
        BOOST_REQUIRE_EQUAL(op.to, buratino.name);
        BOOST_REQUIRE_EQUAL(op.amount, SCORUM_MIN_PRODUCER_REWARD);
    }

    transfer_operation op;
    op.from = buratino;
    op.to = maugli;
    op.amount = SCORUM_MIN_PRODUCER_REWARD / 2;
    push_operation(op);

    {
        operation_map_type buratino_ops
            = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_scr_history_object>(
                buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_scr_history_object>(
                maugli);

        BOOST_REQUIRE_EQUAL(buratino_ops.size(), 2u);
        BOOST_REQUIRE_EQUAL(maugli_ops.size(), 1u);

        BOOST_REQUIRE(buratino_ops[1].op == op);
        BOOST_REQUIRE(maugli_ops[0].op == op);
    }
}

SCORUM_TEST_CASE(check_account_transfer_to_scorumpower_operation_history_test)
{
    actor(initdelegate).create_account(buratino);
    actor(initdelegate).create_account(maugli);

    actor(initdelegate).give_scr(buratino, SCORUM_MIN_PRODUCER_REWARD.amount.value);

    {
        operation_map_type buratino_ops
            = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_sp_history_object>(
                buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_sp_history_object>(
                maugli);

        BOOST_REQUIRE_EQUAL(buratino_ops.size(), 0u);
        BOOST_REQUIRE_EQUAL(maugli_ops.size(), 0u);
    }

    transfer_to_scorumpower_operation op;
    op.from = buratino;
    op.to = maugli;
    op.amount = SCORUM_MIN_PRODUCER_REWARD / 2;
    push_operation(op);

    {
        operation_map_type buratino_ops
            = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_sp_history_object>(
                buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::account_transfers_to_sp_history_object>(
                maugli);

        BOOST_REQUIRE_EQUAL(buratino_ops.size(), 1u);
        BOOST_REQUIRE_EQUAL(maugli_ops.size(), 1u);

        BOOST_REQUIRE(buratino_ops[0].op == op);
        BOOST_REQUIRE(maugli_ops[0].op == op);
    }
}

SCORUM_TEST_CASE(check_get_account_scr_to_scr_transfers)
{
    opetations_type input_ops;

    const size_t over_limit = 10;

    // sam has not been feeded yet

    generate_block();

    operation_map_type ret = _api.get_account_scr_to_scr_transfers(sam, -1, over_limit);
    BOOST_REQUIRE_EQUAL(ret.size(), 0u);

    {
        transfer_operation op;
        op.from = alice.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 10);
        op.memo = "from alice";
        push_operation(op);
        input_ops.push_back(op);
    }

    {
        transfer_operation op;
        op.from = bob.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 20);
        op.memo = "from bob";
        push_operation(op);
        input_ops.push_back(op);
    }

    BOOST_REQUIRE_LT(input_ops.size(), over_limit);

    generate_block();

    // only two
    ret = _api.get_account_scr_to_scr_transfers(sam, -1, over_limit);
    BOOST_REQUIRE_EQUAL(ret.size(), 2u);

    auto itr = ret.end();
    auto record_number = (--itr)->first;

    {
        transfer_operation op;
        op.from = bob.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 33);
        op.memo = "test";
        push_operation(op);
        input_ops.push_back(op);
    }

    BOOST_REQUIRE_LT(input_ops.size(), over_limit);

    SCORUM_REQUIRE_THROW(_api.get_account_scr_to_scr_transfers(sam, -1, 0), fc::exception);

    SCORUM_REQUIRE_THROW(_api.get_account_scr_to_scr_transfers(sam, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH + 1),
                         fc::exception);

    ret = _api.get_account_scr_to_scr_transfers(sam, -1, 1u);
    BOOST_REQUIRE_EQUAL(ret.size(), 1u);

    ret = _api.get_account_scr_to_scr_transfers(sam, record_number + 1, record_number);
    BOOST_REQUIRE_EQUAL(ret.size(), record_number);

    ret = _api.get_account_scr_to_scr_transfers(sam, -1, over_limit);

    saved_operation_vector_type saved_ops;

    for (const auto& val : ret)
    {
        saved_ops.push_back(val);
    }

    BOOST_REQUIRE_EQUAL(input_ops.size(), saved_ops.size());

    auto it = input_ops.begin();
    for (const auto& op_val : saved_ops)
    {
        const auto& saved_op = op_val.second.op;
        saved_op.visit(check_opetation_visitor(*it));

        ++it;
    }
}

SCORUM_TEST_CASE(check_get_account_scr_to_sp_transfers)
{
    opetations_type input_ops;

    const size_t over_limit = 10;

    // sam has not been feeded yet

    generate_block();

    operation_map_type ret = _api.get_account_scr_to_sp_transfers(sam, -1, over_limit);
    BOOST_REQUIRE_EQUAL(ret.size(), 0u);

    {
        transfer_to_scorumpower_operation op;
        op.from = alice.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 10);
        push_operation(op);
        input_ops.push_back(op);
    }

    {
        transfer_to_scorumpower_operation op;
        op.from = bob.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 20);
        push_operation(op);
        input_ops.push_back(op);
    }

    BOOST_REQUIRE_LT(input_ops.size(), over_limit);

    generate_block();

    // only two
    ret = _api.get_account_scr_to_sp_transfers(sam, -1, over_limit);
    BOOST_REQUIRE_EQUAL(ret.size(), 2u);

    auto itr = ret.end();
    auto record_number = (--itr)->first;

    {
        transfer_to_scorumpower_operation op;
        op.from = bob.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 33);
        push_operation(op);
        input_ops.push_back(op);
    }

    BOOST_REQUIRE_LT(input_ops.size(), over_limit);

    SCORUM_REQUIRE_THROW(_api.get_account_scr_to_sp_transfers(sam, -1, 0), fc::exception);

    SCORUM_REQUIRE_THROW(_api.get_account_scr_to_sp_transfers(sam, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH + 1),
                         fc::exception);

    ret = _api.get_account_scr_to_sp_transfers(sam, -1, 1u);
    BOOST_REQUIRE_EQUAL(ret.size(), 1u);

    ret = _api.get_account_scr_to_sp_transfers(sam, record_number + 1, record_number);
    BOOST_REQUIRE_EQUAL(ret.size(), record_number);

    ret = _api.get_account_scr_to_sp_transfers(sam, -1, over_limit);

    saved_operation_vector_type saved_ops;

    for (const auto& val : ret)
    {
        saved_ops.push_back(val);
    }

    BOOST_REQUIRE_EQUAL(input_ops.size(), saved_ops.size());

    auto it = input_ops.begin();
    for (const auto& op_val : saved_ops)
    {
        const auto& saved_op = op_val.second.op;
        saved_op.visit(check_opetation_visitor(*it));

        ++it;
    }
}

SCORUM_TEST_CASE(check_get_account_scr_to_scr_transfers_look_account_conformity)
{
    const int over_limit = 10;

    // sam has not been feeded yet

    generate_block();

    opetations_type input_sam_ops;

    {
        transfer_operation op;
        op.from = alice.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 10);
        op.memo = "from alice";
        push_operation(op);
        input_sam_ops.push_back(op);
    }

    {
        transfer_operation op;
        op.from = bob.name;
        op.to = alice.name;
        op.amount = ASSET_SCR(feed_amount / 11);
        op.memo = "from bob to alice";
        push_operation(op);
    }

    {
        transfer_operation op;
        op.from = alice.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 20);
        op.memo = "from alice (2)";
        push_operation(op);
        input_sam_ops.push_back(op);
    }

    operation_map_type ret = _api.get_account_scr_to_scr_transfers(sam, -1, over_limit);
    BOOST_REQUIRE_EQUAL(ret.size(), 2u);

    _api.get_account_scr_to_scr_transfers(sam, -1, 2u);
    BOOST_REQUIRE_EQUAL(ret.size(), 2u);

    saved_operation_vector_type saved_ops;

    for (const auto& val : ret)
    {
        saved_ops.push_back(val);
    }

    auto it = input_sam_ops.begin();
    for (const auto& op_val : saved_ops)
    {
        const auto& saved_op = op_val.second.op;
        saved_op.visit(check_opetation_visitor(*it));

        ++it;
    }
}

SCORUM_TEST_CASE(check_get_account_history)
{
    opetations_type input_ops;

    {
        transfer_to_scorumpower_operation op;
        op.from = alice.name;
        op.to = bob.name;
        op.amount = ASSET_SCR(feed_amount / 10);
        push_operation(op);
        input_ops.push_back(op);
    }

    {
        transfer_operation op;
        op.from = bob.name;
        op.to = alice.name;
        op.amount = ASSET_SCR(feed_amount / 20);
        op.memo = "test";
        push_operation(op);
        input_ops.push_back(op);
    }

    {
        transfer_to_scorumpower_operation op;
        op.from = alice.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 30);
        push_operation(op);
        input_ops.push_back(op);
    }

    saved_operation_vector_type saved_ops;

    SCORUM_REQUIRE_THROW(_api.get_account_history(alice, -1, 0), fc::exception);

    SCORUM_REQUIRE_THROW(_api.get_account_history(alice, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH + 1), fc::exception);

    operation_map_type ret1 = _api.get_account_history(alice, -1, 1);
    BOOST_REQUIRE_EQUAL(ret1.size(), 1u);

    auto next_page_id = ret1.begin()->first;
    next_page_id--;
    operation_map_type ret2 = _api.get_account_history(alice, next_page_id, 2);
    BOOST_REQUIRE_EQUAL(ret2.size(), 2u);

    for (const auto& val : ret2) // oldest history
    {
        saved_ops.push_back(val);
    }

    for (const auto& val : ret1)
    {
        saved_ops.push_back(val);
    }

    BOOST_REQUIRE_EQUAL(input_ops.size(), saved_ops.size());

    auto it = input_ops.begin();
    for (const auto& op_val : saved_ops)
    {
        const auto& saved_op = op_val.second.op;
        saved_op.visit(check_opetation_visitor(*it));

        ++it;
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(get_account_sp_to_scr_transfers_tests, blockchain_history_tests::history_database_fixture)

SCORUM_TEST_CASE(check_same_acc_finished_transfer)
{
    const int feed_amount = 10 * SCORUM_VESTING_WITHDRAW_INTERVALS;

    BOOST_TEST_MESSAGE("Start withdraw Alice");
    {
        withdraw_scorumpower_operation op;
        op.account = alice.name;
        op.scorumpower = ASSET_SP(feed_amount);
        push_operation(op, fc::ecc::private_key(), false);
    }

    BOOST_TEST_MESSAGE("Generating blocks");
    for (uint32_t ci = 0; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal, true);
    }

    BOOST_TEST_MESSAGE("Check Result");

    auto alice_hist = _api.get_account_sp_to_scr_transfers(alice, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(alice_hist.size(), 1u);
    BOOST_CHECK_EQUAL(alice_hist[0].withdrawn.amount, feed_amount);
    BOOST_CHECK_EQUAL(alice_hist[0].status, scorum::blockchain_history::applied_withdraw_operation::finished);
}

SCORUM_TEST_CASE(check_withdraw_started_but_no_transfers_occured)
{
    const int feed_amount = 10 * SCORUM_VESTING_WITHDRAW_INTERVALS;

    withdraw_scorumpower_operation op;
    op.account = alice.name;
    op.scorumpower = ASSET_SP(feed_amount);
    push_operation(op);

    auto alice_hist = _api.get_account_sp_to_scr_transfers(alice, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(alice_hist.size(), 1u);
    BOOST_CHECK_EQUAL(alice_hist[0].withdrawn.amount, 0u);
    BOOST_CHECK_EQUAL(alice_hist[0].status, scorum::blockchain_history::applied_withdraw_operation::active);
}

SCORUM_TEST_CASE(check_same_acc_transfer_interrupted_by_zero_withdraw)
{
    const int feed_amount = 10 * SCORUM_VESTING_WITHDRAW_INTERVALS;

    BOOST_TEST_MESSAGE("Start withdraw Alice");
    {
        withdraw_scorumpower_operation op;
        op.account = alice.name;
        op.scorumpower = ASSET_SP(feed_amount);
        push_operation(op, fc::ecc::private_key(), false);
    }

    BOOST_TEST_MESSAGE("Generating blocks for a half of intervals");
    auto lhs_intervals = SCORUM_VESTING_WITHDRAW_INTERVALS / 2;

    for (uint32_t ci = 0; ci < lhs_intervals; ++ci)
    {
        auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal, true);
    }

    BOOST_TEST_MESSAGE("Reset withdraw Alice");
    {
        withdraw_scorumpower_operation op;
        op.account = alice.name;
        op.scorumpower = ASSET_SP(0);
        push_operation(op);
    }

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
    generate_blocks(next_withdrawal, true);

    BOOST_TEST_MESSAGE("Check Result");
    auto alice_hist = _api.get_account_sp_to_scr_transfers(alice, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(alice_hist.size(), 2u);
    BOOST_CHECK_EQUAL(alice_hist[1].withdrawn.amount, 0u);
    BOOST_CHECK_EQUAL(alice_hist[1].status, scorum::blockchain_history::applied_withdraw_operation::empty);
    BOOST_CHECK_EQUAL(alice_hist[0].withdrawn.amount, feed_amount * lhs_intervals / SCORUM_VESTING_WITHDRAW_INTERVALS);
    BOOST_CHECK_EQUAL(alice_hist[0].status, scorum::blockchain_history::applied_withdraw_operation::interrupted);
}

SCORUM_TEST_CASE(check_same_acc_transfer_interrupted_by_non_zero_withdraw)
{
    const int feed_amount = 10 * SCORUM_VESTING_WITHDRAW_INTERVALS;

    BOOST_TEST_MESSAGE("Start withdraw Alice");
    {
        withdraw_scorumpower_operation op;
        op.account = alice.name;
        op.scorumpower = ASSET_SP(feed_amount);
        push_operation(op, fc::ecc::private_key(), false);
    }

    BOOST_TEST_MESSAGE("Generating blocks for a half of intervals");
    auto lhs_intervals = SCORUM_VESTING_WITHDRAW_INTERVALS / 2;

    for (uint32_t ci = 0; ci < lhs_intervals; ++ci)
    {
        auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal, true);
    }

    BOOST_TEST_MESSAGE("Reset withdraw Alice");
    {
        withdraw_scorumpower_operation op;
        op.account = alice.name;
        op.scorumpower = ASSET_SP(feed_amount);
        push_operation(op);
    }

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
    generate_blocks(next_withdrawal, true);

    BOOST_TEST_MESSAGE("Check Result");
    auto alice_hist = _api.get_account_sp_to_scr_transfers(alice, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(alice_hist.size(), 2u);
    BOOST_CHECK_EQUAL(alice_hist[1].withdrawn.amount, feed_amount / SCORUM_VESTING_WITHDRAW_INTERVALS);
    BOOST_CHECK_EQUAL(alice_hist[1].status, scorum::blockchain_history::applied_withdraw_operation::active);
    BOOST_CHECK_EQUAL(alice_hist[0].withdrawn.amount, feed_amount * lhs_intervals / SCORUM_VESTING_WITHDRAW_INTERVALS);
    BOOST_CHECK_EQUAL(alice_hist[0].status, scorum::blockchain_history::applied_withdraw_operation::interrupted);
}

SCORUM_TEST_CASE(check_withdraw_status_changing_during_same_acc_transfer)
{
    const int feed_amount = 10 * SCORUM_VESTING_WITHDRAW_INTERVALS;

    BOOST_TEST_MESSAGE("Start withdraw Alice");
    {
        withdraw_scorumpower_operation op;
        op.account = alice.name;
        op.scorumpower = ASSET_SP(feed_amount);
        push_operation(op, fc::ecc::private_key(), false);
    }

    BOOST_TEST_MESSAGE("Generating blocks for a half of intervals");
    auto lhs_intervals = SCORUM_VESTING_WITHDRAW_INTERVALS / 2;
    auto rhs_intervals = SCORUM_VESTING_WITHDRAW_INTERVALS - lhs_intervals;

    for (uint32_t ci = 0; ci < lhs_intervals; ++ci)
    {
        auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal, true);
    }

    BOOST_TEST_MESSAGE("Check Result during withdraw");
    {
        auto alice_hist = _api.get_account_sp_to_scr_transfers(alice, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
        BOOST_REQUIRE_EQUAL(alice_hist.size(), 1u);
        BOOST_CHECK_EQUAL(alice_hist[0].withdrawn.amount,
                          feed_amount * lhs_intervals / SCORUM_VESTING_WITHDRAW_INTERVALS);
        BOOST_CHECK_EQUAL(alice_hist[0].status, scorum::blockchain_history::applied_withdraw_operation::active);
    }

    for (uint32_t ci = 0; ci < rhs_intervals; ++ci)
    {
        auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal, true);
    }

    BOOST_TEST_MESSAGE("Check Result after withdraw finished");
    {
        auto alice_hist = _api.get_account_sp_to_scr_transfers(alice, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
        BOOST_REQUIRE_EQUAL(alice_hist.size(), 1u);
        BOOST_CHECK_EQUAL(alice_hist[0].withdrawn.amount, feed_amount);
        BOOST_CHECK_EQUAL(alice_hist[0].status, scorum::blockchain_history::applied_withdraw_operation::finished);
    }
}

SCORUM_TEST_CASE(check_reroute_to_other_acc_both_acc_setup_withdraw)
{
    const int feed_amount = 10 * SCORUM_VESTING_WITHDRAW_INTERVALS;

    BOOST_TEST_MESSAGE("Start withdraw Bob");
    {
        withdraw_scorumpower_operation op;
        op.account = bob.name;
        op.scorumpower = ASSET_SP(feed_amount);
        push_operation(op, fc::ecc::private_key(), false);
    }

    BOOST_TEST_MESSAGE("Start withdraw Alice");
    {
        withdraw_scorumpower_operation op;
        op.account = alice.name;
        op.scorumpower = ASSET_SP(feed_amount);
        push_operation(op, fc::ecc::private_key(), false);
    }

    BOOST_TEST_MESSAGE("Start routes from Bob");
    {
        set_withdraw_scorumpower_route_to_account_operation op;
        op.from_account = bob.name;
        op.to_account = alice.name;
        op.auto_vest = true;
        op.percent = SCORUM_PERCENT(50);
        push_operation(op, fc::ecc::private_key(), false);
    }

    BOOST_TEST_MESSAGE("Generating blocks");
    for (uint32_t ci = 0; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal, true);
    }

    BOOST_TEST_MESSAGE("Check Result");

    auto bob_hist = _api.get_account_sp_to_scr_transfers(bob, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(bob_hist.size(), 1u);
    BOOST_CHECK_EQUAL(bob_hist[0].withdrawn.amount, feed_amount);
    BOOST_CHECK_EQUAL(bob_hist[0].status, scorum::blockchain_history::applied_withdraw_operation::finished);

    auto alice_hist = _api.get_account_sp_to_scr_transfers(alice, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(alice_hist.size(), 1u);
    BOOST_CHECK_EQUAL(alice_hist[0].withdrawn.amount, feed_amount);
    BOOST_CHECK_EQUAL(alice_hist[0].status, scorum::blockchain_history::applied_withdraw_operation::finished);
}

SCORUM_TEST_CASE(check_reroute_to_other_acc_other_account_do_not_setup_withdraw)
{
    const int feed_amount = 10 * SCORUM_VESTING_WITHDRAW_INTERVALS;

    BOOST_TEST_MESSAGE("Start withdraw Bob");
    {
        withdraw_scorumpower_operation op;
        op.account = bob.name;
        op.scorumpower = ASSET_SP(feed_amount);
        push_operation(op, fc::ecc::private_key(), false);
    }
    BOOST_TEST_MESSAGE("Start routes from Bob");
    {
        set_withdraw_scorumpower_route_to_account_operation op;
        op.from_account = bob.name;
        op.to_account = alice.name;
        op.auto_vest = true;
        op.percent = SCORUM_PERCENT(50);
        push_operation(op, fc::ecc::private_key(), false);
    }

    BOOST_TEST_MESSAGE("Generating blocks");
    for (uint32_t ci = 0; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal, true);
    }

    BOOST_TEST_MESSAGE("Check Result");

    auto bob_hist = _api.get_account_sp_to_scr_transfers(bob, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(bob_hist.size(), 1u);
    BOOST_CHECK_EQUAL(bob_hist[0].withdrawn.amount, feed_amount);
    BOOST_CHECK_EQUAL(bob_hist[0].status, scorum::blockchain_history::applied_withdraw_operation::finished);

    auto alice_hist = _api.get_account_sp_to_scr_transfers(alice, -1, MAX_BLOCKCHAIN_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(alice_hist.size(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()

struct blokchain_history_fixture : public history_database_fixture
{
    blokchain_history_fixture()
        : _blockchain_history_api_ctx(app, API_BLOCKCHAIN_HISTORY, std::make_shared<api_session_data>())
        , blockchain_history_api_call(_blockchain_history_api_ctx)
    {
    }

    api_context _blockchain_history_api_ctx;
    blockchain_history::blockchain_history_api blockchain_history_api_call;
};

BOOST_FIXTURE_TEST_SUITE(blockchain_history_tests, blokchain_history_fixture)

SCORUM_TEST_CASE(check_get_ops_in_block)
{
    opetations_type input_ops;

    generate_block();

    {
        transfer_to_scorumpower_operation op;
        op.from = alice.name;
        op.to = bob.name;
        op.amount = ASSET_SCR(feed_amount / 10);
        push_operation(op, alice.private_key, false);
        input_ops.push_back(op);
    }

    {
        auto signing_key = private_key_type::regenerate(fc::sha256::hash("witness")).get_public_key();
        witness_update_operation op;
        op.owner = alice;
        op.url = "witness creation";
        op.block_signing_key = signing_key;
        push_operation(op, alice.private_key, false);
        input_ops.push_back(op);
    }

    generate_block();

    dynamic_global_property_service_i& dpo_service = db.dynamic_global_property_service();

    saved_operation_vector_type saved_ops;

    // expect transfer_to_scorumpower_operation, witness_update_operation
    operation_map_type ret = blockchain_history_api_call.get_ops_in_block(
        dpo_service.get().head_block_number, blockchain_history::applied_operation_type::not_virt);
    BOOST_REQUIRE_EQUAL(ret.size(), 2u);

    saved_ops.clear();
    for (const auto& val : ret)
    {
        saved_ops.push_back(val);
    }

    auto it = input_ops.begin();
    for (const auto& op_val : saved_ops)
    {
        const auto& saved_op = op_val.second.op;
        saved_op.visit(check_opetation_visitor(*it));

        ++it;
    }

    // expect transfer_to_scorumpower_operation
    ret = blockchain_history_api_call.get_ops_in_block(dpo_service.get().head_block_number,
                                                       blockchain_history::applied_operation_type::market);
    BOOST_REQUIRE_EQUAL(ret.size(), 1u);

    saved_ops.clear();
    for (const auto& val : ret)
    {
        saved_ops.push_back(val);
    }

    it = input_ops.begin();
    for (const auto& op_val : saved_ops)
    {
        const auto& saved_op = op_val.second.op;
        saved_op.visit(check_opetation_visitor(*it));

        ++it;
    }

    // expect transfer_to_scorumpower_operation, witness_update_operation, producer_reward_operation
    ret = blockchain_history_api_call.get_ops_in_block(dpo_service.get().head_block_number,
                                                       blockchain_history::applied_operation_type::all);
    BOOST_REQUIRE_EQUAL(ret.size(), 3u);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(blockchain_history_by_time_tests, blokchain_history_fixture)

SCORUM_TEST_CASE(get_ops_history_by_time_negative_check)
{
    auto timestamp1 = db.head_block_time();
    auto timestamp2 = db.head_block_time() + SCORUM_BLOCK_INTERVAL * 10;

    BOOST_REQUIRE_LT((timestamp2 - timestamp1).to_seconds(), MAX_TIMESTAMP_RANGE_IN_S);

    SCORUM_MESSAGE("Check invalid page settings");

    SCORUM_REQUIRE_THROW(blockchain_history_api_call.get_ops_history_by_time(timestamp1, timestamp2, -1, -1),
                         fc::exception);
    SCORUM_REQUIRE_THROW(blockchain_history_api_call.get_ops_history_by_time(timestamp1, timestamp2, -1,
                                                                             MAX_BLOCKCHAIN_HISTORY_DEPTH + 1),
                         fc::exception);
    SCORUM_REQUIRE_THROW(blockchain_history_api_call.get_ops_history_by_time(timestamp1, timestamp2, 1, 2),
                         fc::exception);

    SCORUM_MESSAGE("Check invalid time settings");

    SCORUM_REQUIRE_THROW(blockchain_history_api_call.get_ops_history_by_time(timestamp2, timestamp1, -1, 1),
                         fc::exception);

    timestamp2 = timestamp1 + MAX_TIMESTAMP_RANGE_IN_S * 2;

    SCORUM_REQUIRE_THROW(blockchain_history_api_call.get_ops_history_by_time(timestamp1, timestamp2, -1, 1),
                         fc::exception);
}

SCORUM_TEST_CASE(get_ops_history_by_time_positive_check)
{
    // Case description:
    //
    // There are two time intervals. First will have transfer_to_scorumpower_operation, transfer_operation operations,
    // second will have witness_update_operation, transfer_to_scorumpower_operation. There are many virtual operations
    // betwing from start of first intreval to end of second intreval. So we check if our operations exist in that
    // intervals

    opetations_type input_ops_timestamp1;

    generate_block();

    auto timestamp1 = db.head_block_time();

    {
        transfer_to_scorumpower_operation op;
        op.from = alice.name;
        op.to = bob.name;
        op.amount = ASSET_SCR(feed_amount / 10);
        push_operation(op, alice.private_key);
        input_ops_timestamp1.push_back(op);
    }

    {
        transfer_operation op;
        op.from = bob.name;
        op.to = alice.name;
        op.amount = ASSET_SCR(feed_amount / 20);
        op.memo = "test";
        push_operation(op, bob.private_key);
        input_ops_timestamp1.push_back(op);
    }

    generate_block();

    auto timestamp2 = db.head_block_time() + SCORUM_BLOCK_INTERVAL * 10;

    generate_blocks(timestamp2);

    opetations_type input_ops_timestamp2;

    {
        auto signing_key = private_key_type::regenerate(fc::sha256::hash("witness")).get_public_key();
        witness_update_operation op;
        op.owner = alice;
        op.url = "witness creation";
        op.block_signing_key = signing_key;
        push_operation(op, alice.private_key);
        input_ops_timestamp2.push_back(op);
    }

    {
        transfer_to_scorumpower_operation op;
        op.from = alice.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 30);
        push_operation(op, alice.private_key);
        input_ops_timestamp2.push_back(op);
    }

    generate_block();

    {
        SCORUM_MESSAGE("Check first time interval = [timestamp1, timestamp2)");

        operation_map_type result = blockchain_history_api_call.get_ops_history_by_time(
            timestamp1, timestamp2 - SCORUM_BLOCK_INTERVAL, -1, 100);

        for (const auto& item : result)
        {
            const auto& op = item.second.op;
            op.visit(view_opetation_visitor());
        }

        check_opetations_list_visitor v(input_ops_timestamp1);

        for (const auto& item : result)
        {
            const auto& op = item.second.op;
            op.visit(v);
        }

        BOOST_CHECK(v.successed());
    }

    {
        SCORUM_MESSAGE("Check second time interval = [timestamp2, now]");

        operation_map_type result
            = blockchain_history_api_call.get_ops_history_by_time(timestamp2, db.head_block_time(), -1, 100);

        for (const auto& item : result)
        {
            const auto& op = item.second.op;
            op.visit(view_opetation_visitor());
        }

        check_opetations_list_visitor v(input_ops_timestamp2);

        for (const auto& item : result)
        {
            const auto& op = item.second.op;
            op.visit(v);
        }

        BOOST_CHECK(v.successed());
    }
}

BOOST_AUTO_TEST_SUITE_END()

struct devcommittee_history_fixture : public devcommittee_fixture::devcommittee_fixture
{
    devcommittee_history_fixture()
        : alice("alice")
        , bob("bob")
        , _devcommittee_history_api_ctx(app, API_DEVCOMMITTEE_HISTORY, std::make_shared<api_session_data>())
        , _devapi(_devcommittee_history_api_ctx)
    {
        init_plugin<scorum::blockchain_history::blockchain_history_plugin>();

        auto genesis = default_genesis_state()
                           .development_sp_supply(ASSET_SP(1000))
                           .development_scr_supply(ASSET_SCR(1000))
                           .generate();
        open_database(genesis);

        generate_block();
        validate_database();

        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, feed_amount);
        actor(initdelegate).give_sp(alice, feed_amount);

        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_scr(bob, feed_amount);
        actor(initdelegate).give_sp(bob, feed_amount);
    }

    const int feed_amount = 99000;

    Actor alice;
    Actor bob;

    api_context _devcommittee_history_api_ctx;
    blockchain_history::devcommittee_history_api _devapi;
};

BOOST_FIXTURE_TEST_SUITE(devcommittee_history_tests, devcommittee_history_fixture)

SCORUM_TEST_CASE(get_history_positive_test)
{
    BOOST_TEST_MESSAGE("Preparing required data");

    devcommittee_add_member(initdelegate, alice, initdelegate);
    devcommittee_change_quorum(initdelegate, exclude_member_quorum, 50u, initdelegate, alice);
    devcommittee_exclude_member(initdelegate, alice, initdelegate);
    devcommittee_withdraw(initdelegate, ASSET_SP(1000), initdelegate);

    wait_withdraw(1);

    devcommittee_transfer(initdelegate, alice, ASSET_SCR(100), initdelegate);

    wait_withdraw(SCORUM_VESTING_WITHDRAW_INTERVALS);

    BOOST_TEST_MESSAGE("Return all");
    {
        auto hist = _devapi.get_history(-1, 100);

        // 1(add) + 1(exclude) + 1(withdraw create) + 1(transfer) + [13(withdrawn 76 SP) + 1(withdrawn 12 SP)] +
        // 1(finishde withdraw)
        BOOST_REQUIRE_EQUAL(hist.size(), 20u);
        BOOST_REQUIRE_NO_THROW(hist[0].op.get<devpool_finished_vesting_withdraw_operation>());
        BOOST_REQUIRE_NO_THROW(hist[19]
                                   .op.get<proposal_virtual_operation>()
                                   .proposal_op.get<development_committee_add_member_operation>());
    }

    BOOST_TEST_MESSAGE("Return last 4 events");
    {
        auto hist = _devapi.get_history(-1, 4);

        BOOST_REQUIRE_EQUAL(hist.size(), 4u);
        BOOST_REQUIRE_NO_THROW(hist[0].op.get<devpool_finished_vesting_withdraw_operation>());
        BOOST_REQUIRE_NO_THROW(hist[3].op.get<devpool_to_devpool_vesting_withdraw_operation>());
    }

    BOOST_TEST_MESSAGE("Return first 3 events");
    {
        auto hist = _devapi.get_history(2, 42);

        BOOST_REQUIRE_EQUAL(hist.size(), 3u);
        BOOST_REQUIRE_NO_THROW(hist[0]
                                   .op.get<proposal_virtual_operation>()
                                   .proposal_op.get<development_committee_exclude_member_operation>());
        BOOST_REQUIRE_NO_THROW(
            hist[2].op.get<proposal_virtual_operation>().proposal_op.get<development_committee_add_member_operation>());
    }
}

SCORUM_TEST_CASE(get_history_negative_test)
{
    BOOST_CHECK_THROW(_devapi.get_history(-1, 0), fc::assert_exception);
    BOOST_CHECK_THROW(_devapi.get_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH + 1), fc::assert_exception);
}

SCORUM_TEST_CASE(get_transfers_positive_test)
{
    BOOST_TEST_MESSAGE("Preparing required data");
    devcommittee_transfer(initdelegate, alice, ASSET_SCR(100), initdelegate);
    devcommittee_change_quorum(initdelegate, exclude_member_quorum, 50u, initdelegate);
    devcommittee_transfer(initdelegate, bob, ASSET_SCR(200), initdelegate);

    BOOST_TEST_MESSAGE("Return all in correct order test");
    {
        auto hist = _devapi.get_scr_to_scr_transfers(-1, 100);

        BOOST_REQUIRE_EQUAL(hist.size(), 2u);
        BOOST_REQUIRE_NO_THROW({
            const auto& op = hist[0]
                                 .op.get<proposal_virtual_operation>()
                                 .proposal_op.get<development_committee_transfer_operation>();
            BOOST_CHECK_EQUAL(op.amount.amount, 200u);
        });
        BOOST_REQUIRE_NO_THROW({
            const auto& op = hist[1]
                                 .op.get<proposal_virtual_operation>()
                                 .proposal_op.get<development_committee_transfer_operation>();
            BOOST_CHECK_EQUAL(op.amount.amount, 100u);
        });
    }
}

SCORUM_TEST_CASE(get_transfers_negative_test)
{
    BOOST_CHECK_THROW(_devapi.get_scr_to_scr_transfers(-1, 0), fc::assert_exception);
    BOOST_CHECK_THROW(_devapi.get_scr_to_scr_transfers(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH + 1), fc::assert_exception);
}

SCORUM_TEST_CASE(withdraw_after_previous_withdraw_finished_test)
{
    devcommittee_withdraw(initdelegate, ASSET_SP(130), initdelegate);
    wait_withdraw(SCORUM_VESTING_WITHDRAW_INTERVALS);

    devcommittee_withdraw(initdelegate, ASSET_SP(260), initdelegate);
    wait_withdraw(1);

    auto hist = _devapi.get_sp_to_scr_transfers(-1, 100);
    BOOST_CHECK_EQUAL(hist.size(), 2u);

    BOOST_CHECK(hist[0].status == blockchain_history::applied_withdraw_operation::active);
    BOOST_CHECK_EQUAL(hist[0].withdrawn.amount, 20u);
    BOOST_CHECK(hist[1].status == blockchain_history::applied_withdraw_operation::finished);
    BOOST_CHECK_EQUAL(hist[1].withdrawn.amount, 130u);
}

SCORUM_TEST_CASE(new_withdraw_interrupt_previous_withdraw_test)
{
    devcommittee_withdraw(initdelegate, ASSET_SP(130), initdelegate);
    wait_withdraw(SCORUM_VESTING_WITHDRAW_INTERVALS / 2); // |13/2| == 6

    devcommittee_withdraw(initdelegate, ASSET_SP(260), initdelegate);
    wait_withdraw(SCORUM_VESTING_WITHDRAW_INTERVALS);

    auto hist = _devapi.get_sp_to_scr_transfers(-1, 100);
    BOOST_CHECK_EQUAL(hist.size(), 2u);

    BOOST_CHECK(hist[0].status == blockchain_history::applied_withdraw_operation::finished);
    BOOST_CHECK_EQUAL(hist[0].withdrawn.amount, 260u);
    BOOST_CHECK(hist[1].status == blockchain_history::applied_withdraw_operation::interrupted);
    BOOST_CHECK_EQUAL(hist[1].withdrawn.amount, 60u);
}

SCORUM_TEST_CASE(zero_withdraw_interrupt_previous_withdraw_test)
{
    devcommittee_withdraw(initdelegate, ASSET_SP(130), initdelegate);
    wait_withdraw(SCORUM_VESTING_WITHDRAW_INTERVALS / 2); // |13/2| == 6

    devcommittee_withdraw(initdelegate, ASSET_SP(0), initdelegate);
    wait_withdraw(SCORUM_VESTING_WITHDRAW_INTERVALS);

    auto hist = _devapi.get_sp_to_scr_transfers(-1, 100);
    BOOST_CHECK_EQUAL(hist.size(), 2u);

    BOOST_CHECK(hist[0].status == blockchain_history::applied_withdraw_operation::empty);
    BOOST_CHECK_EQUAL(hist[0].withdrawn.amount, 0u);
    BOOST_CHECK(hist[1].status == blockchain_history::applied_withdraw_operation::interrupted);
    BOOST_CHECK_EQUAL(hist[1].withdrawn.amount, 60u);
}

SCORUM_TEST_CASE(get_withrdaw_negative_test)
{
    BOOST_CHECK_THROW(_devapi.get_sp_to_scr_transfers(-1, 0), fc::assert_exception);
    BOOST_CHECK_THROW(_devapi.get_sp_to_scr_transfers(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH + 1), fc::assert_exception);
}

SCORUM_TEST_CASE(zero_withdraw_after_previous_withdraw_finished_should_throw_test)
{
    devcommittee_withdraw(initdelegate, ASSET_SP(130), initdelegate);
    wait_withdraw(SCORUM_VESTING_WITHDRAW_INTERVALS);

    BOOST_REQUIRE_THROW(devcommittee_withdraw(initdelegate, ASSET_SP(0), initdelegate), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(get_ops_history_tests)

using namespace blockchain_history;

struct chain_type : public database_fixture::database_integration_fixture
{
    void init_blockchain_history_plugin()
    {
        this->init_plugin<scorum::blockchain_history::blockchain_history_plugin>();
    }

    virtual ~chain_type() override
    {
    }
};

struct fixture
{
    Actor alice = Actor("alice");
    Actor bob = Actor("bob");

    chain_type _chain;
    blockchain_history_api api;

    fixture()
        : api(api_context(_chain.app, API_BLOCKCHAIN_HISTORY, std::make_shared<api_session_data>()))
    {
        _chain.init_blockchain_history_plugin();
        _chain.open_database(create_genesis());
        BOOST_REQUIRE_EQUAL(0u, _chain.db.head_block_num());
    }

private:
    genesis_state_type create_genesis()
    {
        alice.scorum(ASSET_SCR(100));
        alice.scorumpower(ASSET_SP(100));

        Actor witness(TEST_INIT_DELEGATE_NAME);
        witness.scorum(ASSET_SCR(100));
        witness.scorumpower(ASSET_SP(100));

        const auto accounts_initial_supply = alice.scr_amount + witness.scr_amount;

        static const asset registration_bonus = ASSET_SCR(100);
        registration_stage single_stage{ 1u, 1u, 100u };

        return Genesis::create()
            .accounts_supply(accounts_initial_supply)
            .rewards_supply(TEST_REWARD_INITIAL_SUPPLY)
            .witnesses(witness)
            .accounts(alice, bob)
            .registration_supply(registration_bonus * 100)
            .registration_bonus(registration_bonus)
            .registration_schedule(single_stage)
            .committee(alice)
            .dev_committee(alice)
            .generate();
    }
};

BOOST_FIXTURE_TEST_CASE(get_all_operations, fixture)
{
    {
        BOOST_REQUIRE_EQUAL(0u, _chain.db.head_block_num());

        BOOST_CHECK_EQUAL(0u,
                          api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::virt).size());

        BOOST_CHECK_EQUAL(0u,
                          api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::market).size());

        BOOST_CHECK_EQUAL(0u,
                          api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::all).size());

        BOOST_CHECK_EQUAL(
            0u, api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::not_virt).size());
    }

    {
        // generate producer reward operation
        _chain.generate_block();

        BOOST_CHECK_EQUAL(1u, _chain.db.head_block_num());

        BOOST_CHECK_EQUAL(1u,
                          api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::virt).size());

        BOOST_CHECK_EQUAL(0u,
                          api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::market).size());

        BOOST_CHECK_EQUAL(1u,
                          api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::all).size());

        BOOST_CHECK_EQUAL(
            0u, api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::not_virt).size());
    }

    {
        scorum::protocol::transfer_operation op;
        op.from = alice.name;
        op.to = bob.name;
        op.amount = ASSET_SCR(1);

        _chain.push_operation(op, alice.private_key);

        BOOST_CHECK_EQUAL(2u, _chain.db.head_block_num());

        BOOST_CHECK_EQUAL(2u,
                          api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::virt).size());

        BOOST_CHECK_EQUAL(1u,
                          api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::market).size());

        BOOST_CHECK_EQUAL(3u,
                          api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::all).size());

        BOOST_CHECK_EQUAL(
            1u, api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::not_virt).size());
    }

    {
        scorum::protocol::comment_operation op;
        op.author = alice.name;
        op.permlink = "permlink";
        op.body = "body";
        op.title = "title";
        op.parent_permlink = "parent-permlink";

        try
        {
            _chain.push_operation(op, alice.private_key);
        }
        FC_LOG_AND_RETHROW()

        // clang-format off
        auto all = api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::all);
        auto virt_operations = api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::virt);
        auto market_operations = api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::market);
        auto not_virt_operations = api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::not_virt);
        // clang-format on

        BOOST_REQUIRE_EQUAL(3u, _chain.db.head_block_num());

        BOOST_REQUIRE_EQUAL(_chain.db.head_block_num(), virt_operations.size());

        BOOST_REQUIRE_EQUAL(1u, market_operations.size());

        BOOST_REQUIRE_EQUAL(2u, not_virt_operations.size());

        BOOST_REQUIRE_EQUAL(all.size(), virt_operations.size() + not_virt_operations.size());
    }

    {
        auto history = api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH, applied_operation_type::all);

        BOOST_CHECK_EQUAL(R"(["producer_reward",{"producer":"initdelegate","reward":"0.000000010 SP"}])",
                          fc::json::to_string(history[0].op));

        BOOST_CHECK_EQUAL(R"(["transfer",{"from":"alice","to":"bob","amount":"0.000000001 SCR","memo":""}])",
                          fc::json::to_string(history[1].op));

        BOOST_CHECK_EQUAL(R"(["producer_reward",{"producer":"initdelegate","reward":"0.000000010 SP"}])",
                          fc::json::to_string(history[2].op));

        BOOST_CHECK_EQUAL(
            R"(["comment",{"parent_author":"","parent_permlink":"parent-permlink","author":"alice","permlink":"permlink","title":"title","body":"body","json_metadata":""}])",
            fc::json::to_string(history[3].op));

        BOOST_CHECK_EQUAL(R"(["producer_reward",{"producer":"initdelegate","reward":"0.000000010 SP"}])",
                          fc::json::to_string(history[4].op));
    }
}

struct single_operation_fixture : fixture
{
    single_operation_fixture()
    {
        scorum::protocol::transfer_operation op;
        op.from = alice.name;
        op.to = bob.name;
        op.amount = ASSET_SCR(1);

        _chain.push_operation(op);
    }
};

BOOST_FIXTURE_TEST_CASE(get_one_operation_when_history_was_empty, single_operation_fixture)
{
    try
    {
        BOOST_REQUIRE_EQUAL(1u, _chain.db.head_block_num());

        auto check = [&](const std::map<uint32_t, applied_operation>& hist) {
            BOOST_REQUIRE_EQUAL(1u, hist.size());

            auto value = *hist.begin();

            BOOST_CHECK_EQUAL(0u, value.first);
            BOOST_CHECK_EQUAL(1u, value.second.block);
            BOOST_CHECK_EQUAL(R"(["transfer",{"from":"alice","to":"bob","amount":"0.000000001 SCR","memo":""}])",
                              fc::json::to_string(value.second.op));
        };

        {
            auto hist = api.get_ops_history(1, 1, applied_operation_type::not_virt);
            check(hist);
        }

        {
            auto hist = api.get_ops_history(static_cast<uint32_t>(-1), 1, applied_operation_type::not_virt);
            check(hist);
        }

        {
            auto hist = api.get_ops_history(100, 100, applied_operation_type::not_virt);
            check(hist);
        }

        {
            auto hist = api.get_ops_history(2, 1, applied_operation_type::not_virt);
            check(hist);
        }
    }
    FC_LOG_AND_RETHROW()
}

struct two_operations_fixture : fixture
{
    two_operations_fixture()
    {
        {
            scorum::protocol::transfer_operation op;
            op.from = alice.name;
            op.to = bob.name;
            op.amount = ASSET_SCR(1);
            _chain.push_operation(op);
        }

        {
            scorum::protocol::transfer_operation op;
            op.from = bob.name;
            op.to = alice.name;
            op.amount = ASSET_SCR(1);
            _chain.push_operation(op);
        }
    }
};

BOOST_FIXTURE_TEST_CASE(get_first_and_last_operation, two_operations_fixture)
{
    try
    {
        BOOST_REQUIRE_EQUAL(2u, _chain.db.head_block_num());

        auto get_first_operation_test = [&]() {
            auto hist = api.get_ops_history(1, 1, applied_operation_type::not_virt);
            BOOST_REQUIRE_EQUAL(1u, hist.size());

            auto value = *hist.begin();

            // operation id
            BOOST_CHECK_EQUAL(0u, value.first);

            // block number
            BOOST_CHECK_EQUAL(1u, value.second.block);

            // validate operation
            BOOST_CHECK_EQUAL(R"(["transfer",{"from":"alice","to":"bob","amount":"0.000000001 SCR","memo":""}])",
                              fc::json::to_string(value.second.op));
        };

        auto get_last_operation_test = [&]() {
            auto hist = api.get_ops_history(static_cast<uint32_t>(-1), 1, applied_operation_type::not_virt);
            BOOST_REQUIRE_EQUAL(1u, hist.size());

            auto value = *hist.begin();

            // operation id
            BOOST_CHECK_EQUAL(1u, value.first);

            // block number
            BOOST_CHECK_EQUAL(2u, value.second.block);

            // validate operation
            BOOST_CHECK_EQUAL(
                R"(["transfer",{"from":"bob","to":"alice","amount":"0.000000001 SCR","memo":""}])",
                fc::json::to_string(value.second.op));
        };

        auto get_all_two_opeations_test = [&]() {
            auto hist = api.get_ops_history(2, 2, applied_operation_type::not_virt);
            BOOST_REQUIRE_EQUAL(2u, hist.size());

            auto first_operation = *hist.begin();
            auto second_operation = *(++hist.begin());

            // operation id
            BOOST_CHECK_EQUAL(0u, first_operation.first);
            BOOST_CHECK_EQUAL(1u, second_operation.first);

            // block number
            BOOST_CHECK_EQUAL(1u, first_operation.second.block);
            BOOST_CHECK_EQUAL(2u, second_operation.second.block);

            // validate operation
            BOOST_CHECK_EQUAL(
                R"(["transfer",{"from":"alice","to":"bob","amount":"0.000000001 SCR","memo":""}])",
                fc::json::to_string(first_operation.second.op));

            BOOST_CHECK_EQUAL(
                R"(["transfer",{"from":"bob","to":"alice","amount":"0.000000001 SCR","memo":""}])",
                fc::json::to_string(second_operation.second.op));
        };

        get_first_operation_test();
        get_last_operation_test();
        get_all_two_opeations_test();

        SCORUM_REQUIRE_THROW(api.get_ops_history(-1, 0, applied_operation_type::market), fc::assert_exception);
        SCORUM_REQUIRE_THROW(api.get_ops_history(-1, MAX_BLOCKCHAIN_HISTORY_DEPTH + 1, applied_operation_type::market),
                             fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace blockchain_history_tests
