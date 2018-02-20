#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>

#include <scorum/protocol/scorum_operations.hpp>

using namespace scorum::protocol;

BOOST_AUTO_TEST_SUITE(proposal_create_operation_tests)

BOOST_AUTO_TEST_CASE(serialize_proposal_create_operation_to_json)
{
    add_member_operation add_member_op;
    add_member_op.account_name = "alice";

    proposal_create_operation2 op;
    op.operation = add_member_op;

    BOOST_CHECK_EQUAL(R"({"creator":"","lifetime_sec":0,"operation":["add_member",{"account_name":"alice"}]})",
                      fc::json::to_string(op));
}

BOOST_AUTO_TEST_CASE(deserialize_proposal_create_operation_from_json)
{
    fc::variant v = fc::json::from_string(
        R"({"creator":"","lifetime_sec":0,"operation":["add_member",{"account_name":"alice"}]})");

    proposal_create_operation2 op;
    fc::from_variant(v, op);

    add_member_operation& add_member_op = op.operation.get<add_member_operation>();

    BOOST_CHECK_EQUAL("alice", add_member_op.account_name);
}

BOOST_AUTO_TEST_SUITE_END()
