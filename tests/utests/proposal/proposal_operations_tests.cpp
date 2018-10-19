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

BOOST_AUTO_TEST_CASE(serialize_devpool_proposal_create_operation_to_json)
{
    development_committee_withdraw_vesting_operation withdraw_vesting_op;
    withdraw_vesting_op.vesting_shares = asset(10000000000, SP_SYMBOL);

    proposal_create_operation proposal_create_op;
    proposal_create_op.operation = withdraw_vesting_op;
    proposal_create_op.creator = "initdelegate";
    proposal_create_op.lifetime_sec = 86400;

    BOOST_CHECK_EQUAL(
        R"({"creator":"initdelegate","lifetime_sec":86400,"operation":["development_committee_withdraw_vesting",{"vesting_shares":"10.000000000 SP"}]})",
        fc::json::to_string(proposal_create_op));
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

BOOST_AUTO_TEST_CASE(serialize_devpool_proposal_create_operation_to_hex)
{
    development_committee_withdraw_vesting_operation withdraw_vesting_op;
    withdraw_vesting_op.vesting_shares = asset(10000000000, SP_SYMBOL);

    proposal_create_operation proposal_create_op;
    proposal_create_op.operation = withdraw_vesting_op;
    proposal_create_op.creator = "initdelegate";
    proposal_create_op.lifetime_sec = 86400;

    scorum::protocol::operation op = proposal_create_op;

    BOOST_CHECK_EQUAL("1d0c696e697464656c6567617465805101000600e40b54020000000953500000000000", utils::to_hex(op));
}

BOOST_AUTO_TEST_CASE(serialize_development_committee_empower_advertising_moderator_operation_to_hex)
{
    development_committee_empower_advertising_moderator_operation empower_moderator_op;
    empower_moderator_op.account = "alice";

    proposal_create_operation proposal_create_op;
    proposal_create_op.operation = empower_moderator_op;
    proposal_create_op.creator = "initdelegate";
    proposal_create_op.lifetime_sec = 86400;

    scorum::protocol::operation op = proposal_create_op;

    BOOST_CHECK_EQUAL("1d0c696e697464656c6567617465805101000805616c696365", utils::to_hex(op));
}

BOOST_AUTO_TEST_CASE(serialize_development_committee_empower_betting_moderator_operation_to_hex)
{
    development_committee_empower_betting_moderator_operation empower_moderator_op;
    empower_moderator_op.account = "alice";

    proposal_create_operation proposal_create_op;
    proposal_create_op.operation = empower_moderator_op;
    proposal_create_op.creator = "initdelegate";
    proposal_create_op.lifetime_sec = 86400;

    scorum::protocol::operation op = proposal_create_op;

    BOOST_CHECK_EQUAL("1d0c696e697464656c6567617465805101000b05616c696365", utils::to_hex(op));
}

BOOST_AUTO_TEST_CASE(serialize_development_committee_change_betting_resolve_delay_operation_to_hex)
{
    development_committee_change_betting_resolve_delay_operation resolve_delay_op;
    resolve_delay_op.delay_sec = 10;

    proposal_create_operation proposal_create_op;
    proposal_create_op.operation = resolve_delay_op;
    proposal_create_op.creator = "initdelegate";
    proposal_create_op.lifetime_sec = 86400;

    scorum::protocol::operation op = proposal_create_op;

    BOOST_CHECK_EQUAL("1d0c696e697464656c6567617465805101000c0a000000", utils::to_hex(op));
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
