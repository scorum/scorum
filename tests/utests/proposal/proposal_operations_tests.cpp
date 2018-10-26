#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>

#include <scorum/protocol/operations.hpp>

#include <detail.hpp>

using detail::to_hex;

namespace proposal_operations_tests {

using namespace scorum::protocol;

BOOST_AUTO_TEST_SUITE(proposal_create_operation_tests)

BOOST_AUTO_TEST_CASE(serialize_proposal_create_operation_to_json)
{
    registration_committee_add_member_operation add_member_op;
    add_member_op.account_name = "alice";

    proposal_create_operation op;
    op.operation = add_member_op;

    BOOST_CHECK_EQUAL(
        R"({"creator":"","lifetime_sec":0,"operation":["registration_committee_add_member",{"account_name":"alice"}]})",
        fc::json::to_string(op));
}

BOOST_AUTO_TEST_CASE(deserialize_proposal_create_operation_from_json)
{
    fc::variant v = fc::json::from_string(
        R"({"creator":"","lifetime_sec":0,"operation":["registration_committee_add_member",{"account_name":"alice"}]})");

    proposal_create_operation op;
    fc::from_variant(v, op);

    registration_committee_add_member_operation& add_member_op
        = op.operation.get<registration_committee_add_member_operation>();

    BOOST_CHECK_EQUAL("alice", add_member_op.account_name);
}

BOOST_AUTO_TEST_CASE(serialize_proposal_create_operation_to_hex)
{
    registration_committee_add_member_operation add_member_op;
    add_member_op.account_name = "alice";

    proposal_create_operation proposal_create_op;
    proposal_create_op.operation = add_member_op;

    scorum::protocol::operation op = proposal_create_op;

    BOOST_CHECK_EQUAL("1d00000000000005616c696365", to_hex(op));
}

BOOST_AUTO_TEST_CASE(deserialize_proposal_create_operation_from_hex)
{
    const std::string hex_str = "1d00000000000005616c696365";

    std::vector<char> buffer;
    buffer.resize(hex_str.size() / 2);

    BOOST_REQUIRE_EQUAL(hex_str.size(), buffer.size() * 2);

    fc::from_hex(hex_str, buffer.data(), buffer.size());

    operation op;

    fc::raw::unpack<scorum::protocol::operation>(buffer, op);

    proposal_create_operation& proposal_create_op = op.get<proposal_create_operation>();

    registration_committee_add_member_operation& add_member_op
        = proposal_create_op.operation.get<registration_committee_add_member_operation>();

    BOOST_CHECK_EQUAL("alice", add_member_op.account_name);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace proposal_operations_tests
