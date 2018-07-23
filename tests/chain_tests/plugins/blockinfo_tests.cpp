#include <boost/test/unit_test.hpp>
#include <scorum/blockchain_history/blockchain_history_api.hpp>
#include <scorum/blockchain_history/blockchain_history_plugin.hpp>
#include <scorum/blockchain_history/api_objects.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/common_api/config.hpp>

#include "database_trx_integration.hpp"

#include "operation_check.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;

namespace blockinfo_tests {

using block_map_type = std::map<uint32_t, scorum::blockchain_history::signed_block_api_obj>;
using blockheader_map_type = std::map<uint32_t, block_header>;

struct blockinfo_database_fixture : public database_fixture::database_trx_integration_fixture
{
    blockinfo_database_fixture()
        : alice("alice")
        , bob("bob")
        , _api_ctx(app, API_BLOCKCHAIN_HISTORY, std::make_shared<api_session_data>())
        , _api_call(_api_ctx)
        , dpo_service(db.dynamic_global_property_service())
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
    }

    const int feed_amount = 99000;

    Actor alice;
    Actor bob;

    api_context _api_ctx;
    scorum::blockchain_history::blockchain_history_api _api_call;

    dynamic_global_property_service_i& dpo_service;
};
}

BOOST_FIXTURE_TEST_SUITE(blockinfo_tests, blockinfo_tests::blockinfo_database_fixture)

SCORUM_TEST_CASE(check_get_blocks_history_test)
{
    operation input_op;

    generate_block();

    {
        transfer_to_scorumpower_operation op;
        op.from = alice.name;
        op.to = bob.name;
        op.amount = ASSET_SCR(feed_amount / 10);
        push_operation(op, alice.private_key, false);
        input_op = op;
    }

    generate_block(); // include transfer_to_scorumpower_operation
    auto transfer_block_num = dpo_service.get().head_block_number;

    generate_blocks(40); // move to block_log

    SCORUM_REQUIRE_THROW(_api_call.get_blocks_history(-1, 0), fc::exception);

    SCORUM_REQUIRE_THROW(_api_call.get_blocks_history(-1, MAX_BLOCKS_HISTORY_DEPTH + 1), fc::exception);

    auto head_block_number = dpo_service.get().head_block_number;
    BOOST_REQUIRE_LE(head_block_number, MAX_BLOCKS_HISTORY_DEPTH);

    blockinfo_tests::block_map_type ret = _api_call.get_blocks_history(-1, MAX_BLOCKS_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(ret.size(), head_block_number);
    BOOST_REQUIRE_EQUAL(ret.rbegin()->first, head_block_number);

    ret = _api_call.get_blocks_history(transfer_block_num, 1);
    BOOST_REQUIRE_EQUAL(ret.begin()->first, transfer_block_num);
    scorum::blockchain_history::signed_block_api_obj block_api = ret.begin()->second;

    BOOST_REQUIRE_EQUAL(block_api.transactions.size(), 1u);

    const operation& seved_op = block_api.transactions[0].operations[0];

    seved_op.visit(operation_tests::check_opetation_visitor(input_op));
}

SCORUM_TEST_CASE(get_block_headers_history_test)
{
    generate_block();

    generate_blocks(40); // move to block_log

    SCORUM_REQUIRE_THROW(_api_call.get_block_headers_history(-1, 0), fc::exception);

    SCORUM_REQUIRE_THROW(_api_call.get_block_headers_history(-1, MAX_BLOCKS_HISTORY_DEPTH + 1), fc::exception);

    auto head_block_number = dpo_service.get().head_block_number;
    BOOST_REQUIRE_LE(head_block_number, MAX_BLOCKS_HISTORY_DEPTH);

    blockinfo_tests::blockheader_map_type ret = _api_call.get_block_headers_history(-1, MAX_BLOCKS_HISTORY_DEPTH);
    BOOST_REQUIRE_EQUAL(ret.size(), head_block_number);
    BOOST_REQUIRE_EQUAL(ret.rbegin()->first, head_block_number);
}

BOOST_AUTO_TEST_SUITE_END()
