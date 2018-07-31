#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/app/api_context.hpp>

#include <scorum/blockchain_history/blockchain_history_plugin.hpp>
#include <scorum/blockchain_history/schema/account_history_object.hpp>
#include <scorum/blockchain_history/schema/applied_operation.hpp>

#include <scorum/blockchain_history/account_history_api.hpp>
#include <scorum/blockchain_history/blockchain_history_api.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/common_api/config.hpp>

#include "database_trx_integration.hpp"

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
        const auto& idx = db.get_index<blockchain_history::history_index<history_object_type>>()
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
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
    operation_map_type buratino_sp_ops
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);

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
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
    operation_map_type buratino_sp_ops
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);

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
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
    operation_map_type buratino_sp_ops
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);

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
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(maugli);

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
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(maugli);

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
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(maugli);

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
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(maugli);

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

SCORUM_TEST_CASE(check_get_ops_history)
{
    opetations_type input_ops;

    generate_block();

    {
        transfer_to_scorumpower_operation op;
        op.from = alice.name;
        op.to = bob.name;
        op.amount = ASSET_SCR(feed_amount / 10);
        push_operation(op, alice.private_key);
        input_ops.push_back(op);
    }

    {
        transfer_operation op;
        op.from = bob.name;
        op.to = alice.name;
        op.amount = ASSET_SCR(feed_amount / 20);
        op.memo = "test";
        push_operation(op, bob.private_key);
        input_ops.push_back(op);
    }

    {
        auto signing_key = private_key_type::regenerate(fc::sha256::hash("witness")).get_public_key();
        witness_update_operation op;
        op.owner = alice;
        op.url = "witness creation";
        op.block_signing_key = signing_key;
        push_operation(op, alice.private_key);
    }

    {
        transfer_to_scorumpower_operation op;
        op.from = alice.name;
        op.to = sam.name;
        op.amount = ASSET_SCR(feed_amount / 30);
        push_operation(op, alice.private_key);
        input_ops.push_back(op);
    }

    saved_operation_vector_type saved_ops;

    SCORUM_REQUIRE_THROW(
        blockchain_history_api_call.get_ops_history(-1, 0, blockchain_history::applied_operation_type::market),
        fc::exception);

    SCORUM_REQUIRE_THROW(blockchain_history_api_call.get_ops_history(
                             -1, MAX_BLOCKCHAIN_HISTORY_DEPTH + 1, blockchain_history::applied_operation_type::market),
                         fc::exception);

    operation_map_type ret1
        = blockchain_history_api_call.get_ops_history(-1, 1, blockchain_history::applied_operation_type::market);
    BOOST_REQUIRE_EQUAL(ret1.size(), 1u);

    auto next_page_id = ret1.begin()->first;
    next_page_id--;
    operation_map_type ret2 = blockchain_history_api_call.get_ops_history(
        next_page_id, 2, blockchain_history::applied_operation_type::market);
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

    // emit producer_reward_operation
    generate_block();

    ret2 = blockchain_history_api_call.get_ops_history(-1, 1, blockchain_history::applied_operation_type::virt);
    BOOST_REQUIRE_EQUAL(ret2.size(), 1u);

    {
        const auto& saved_op = ret2.begin()->second.op;

        BOOST_REQUIRE(is_virtual_operation(saved_op));
    }

    ret2 = blockchain_history_api_call.get_ops_history(-1, 1, blockchain_history::applied_operation_type::all);
    BOOST_REQUIRE_EQUAL(ret2.size(), 1u);

    {
        const auto& saved_op = ret2.begin()->second.op;

        BOOST_REQUIRE(is_virtual_operation(saved_op));
    }
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

} // namespace blockchain_history_tests
