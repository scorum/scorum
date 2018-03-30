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

#include "database_trx_integration.hpp"

#include <scorum/protocol/operations.hpp>

#include "operation_check.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;
using fc::string;

using operation_map_type = std::map<uint32_t, blockchain_history::applied_operation>;

namespace blockchain_history_tests {

struct history_database_fixture : public database_fixture::database_trx_integration_fixture
{
    std::shared_ptr<scorum::blockchain_history::blockchain_history_plugin> _plugin;

    history_database_fixture()
        : buratino("buratino")
        , maugli("maugli")
        , alice("alice")
        , bob("bob")
        , sam("sam")
        , _account_history_api_ctx(app, "account_history_api", std::make_shared<api_session_data>())
        , account_history_api_call(_account_history_api_ctx)
    {
        boost::program_options::variables_map options;

        _plugin = app.register_plugin<scorum::blockchain_history::blockchain_history_plugin>();
        _plugin->plugin_initialize(options);

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
        actor(initdelegate).give_scr(sam, feed_amount);
        actor(initdelegate).give_sp(sam, feed_amount);
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
    blockchain_history::account_history_api account_history_api_call;
};

} // namespace blockchain_history_tests

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

SCORUM_TEST_CASE(check_get_account_history)
{
    using input_operation_vector_type = std::vector<operation>;
    input_operation_vector_type input_ops;

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

    using saved_operation_vector_type = std::vector<operation_map_type::value_type>;
    saved_operation_vector_type saved_ops;

    SCORUM_REQUIRE_THROW(account_history_api_call.get_account_history(alice, -1, 0), fc::exception);

    static const uint32_t max_history_depth = 100;

    SCORUM_REQUIRE_THROW(account_history_api_call.get_account_history(alice, -1, max_history_depth + 1), fc::exception);

    operation_map_type ret1 = account_history_api_call.get_account_history(alice, -1, 1);
    BOOST_REQUIRE_EQUAL(ret1.size(), 1u);

    auto next_page_id = ret1.begin()->first;
    next_page_id--;
    operation_map_type ret2 = account_history_api_call.get_account_history(alice, next_page_id, 2);
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
        saved_op.visit(operation_tests::check_saved_opetations_visitor(*it));

        ++it;
    }
}

BOOST_AUTO_TEST_SUITE_END()

namespace blockchain_history_tests {
struct blokchain_not_virtual_history_database_fixture : public history_database_fixture
{
    blokchain_not_virtual_history_database_fixture()
        : _blockchain_history_api_ctx(app, "blockchain_history_api", std::make_shared<api_session_data>())
        , blockchain_history_api_call(_blockchain_history_api_ctx)
    {
    }

    api_context _blockchain_history_api_ctx;
    blockchain_history::blockchain_history_api blockchain_history_api_call;
};
} // namespace blockchain_history_tests

BOOST_FIXTURE_TEST_SUITE(blockchain_history_tests,
                         blockchain_history_tests::blokchain_not_virtual_history_database_fixture)

SCORUM_TEST_CASE(check_get_ops_in_block)
{
    using input_operation_vector_type = std::vector<operation>;
    input_operation_vector_type input_ops;

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

    using saved_operation_vector_type = std::vector<operation_map_type::value_type>;
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
        saved_op.visit(operation_tests::check_saved_opetations_visitor(*it));

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
        saved_op.visit(operation_tests::check_saved_opetations_visitor(*it));

        ++it;
    }

    // expect transfer_to_scorumpower_operation, witness_update_operation, producer_reward_operation
    ret = blockchain_history_api_call.get_ops_in_block(dpo_service.get().head_block_number,
                                                       blockchain_history::applied_operation_type::all);
    BOOST_REQUIRE_EQUAL(ret.size(), 3u);
}

SCORUM_TEST_CASE(check_get_ops_history)
{
    using input_operation_vector_type = std::vector<operation>;
    input_operation_vector_type input_ops;

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

    using saved_operation_vector_type = std::vector<operation_map_type::value_type>;
    saved_operation_vector_type saved_ops;

    SCORUM_REQUIRE_THROW(
        blockchain_history_api_call.get_ops_history(-1, 0, blockchain_history::applied_operation_type::market),
        fc::exception);

    static const uint32_t max_history_depth = 100;

    SCORUM_REQUIRE_THROW(blockchain_history_api_call.get_ops_history(
                             -1, max_history_depth + 1, blockchain_history::applied_operation_type::market),
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
        saved_op.visit(operation_tests::check_saved_opetations_visitor(*it));

        ++it;
    }
}

BOOST_AUTO_TEST_SUITE_END()
