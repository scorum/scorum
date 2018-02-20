#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>

#include <scorum/protocol/operations.hpp>

#include "utils.hpp"

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

BOOST_AUTO_TEST_CASE(serialize_proposal_create_operation_to_hex)
{
    add_member_operation add_member_op;
    add_member_op.account_name = "alice";

    proposal_create_operation2 proposal_create_op;
    proposal_create_op.operation = add_member_op;

    scorum::protocol::operation op = proposal_create_op;

    BOOST_CHECK_EQUAL("2000000000000005616c696365", utils::to_hex(op));
}

std::string to_hex(const std::vector<char>& data)
{
    std::string r;
    const char* to_hex = "0123456789abcdef";
    for (uint8_t c : data)
    {
        r += to_hex[(c >> 4)];
        r += to_hex[(c & 0x0f)];
    }

    return r;
}

BOOST_AUTO_TEST_CASE(deserialize_proposal_create_operation_from_hex)
{
    //    add_member_operation add_member_op;
    //    add_member_op.account_name = "alice";

    //    proposal_create_operation2 op;
    //    op.operation = add_member_op;

    //    std::vector<char> buffer = fc::raw::pack(op);

    //    BOOST_CHECK_EQUAL(12u, buffer.size());

    const std::string hex_str = "2000000000000005616c696365";

    std::vector<char> buffer;
    buffer.resize(hex_str.size() / 2);

    BOOST_REQUIRE_EQUAL(hex_str.size(), buffer.size() * 2);

    fc::from_hex(hex_str, buffer.data(), buffer.size());

    operation op;

    fc::raw::unpack<scorum::protocol::operation>(buffer, op);

    proposal_create_operation2& proposal_create_op = op.get<proposal_create_operation2>();

    add_member_operation& add_member_op = proposal_create_op.operation.get<add_member_operation>();

    BOOST_CHECK_EQUAL("alice", add_member_op.account_name);
}

BOOST_AUTO_TEST_SUITE_END()
