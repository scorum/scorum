#include <boost/test/unit_test.hpp>

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/operations.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <fc/io/json.hpp>

#include "detail.hpp"
#include "defines.hpp"

using ::detail::from_hex;
using ::detail::to_hex;

namespace {
using namespace std::string_literals;

using scorum::protocol::asset;
using scorum::protocol::create_budget_operation;

BOOST_AUTO_TEST_SUITE(budget_serialization_tests)

SCORUM_TEST_CASE(serialize_create_budget_operation_to_binary_test)
{
    create_budget_operation op;
    op.uuid = boost::uuids::string_generator()("6DCD3132-E5DF-480A-89A8-91984BCA0A09");
    op.type = scorum::protocol::budget_type::post;
    op.owner = "initdelegate";
    op.balance = ASSET("10.000000000 SCR");
    op.start = fc::time_point_sec::from_iso_string("2018-08-03T10:12:43");
    op.deadline = fc::time_point_sec::from_iso_string("2018-08-03T10:13:13");
    op.json_metadata = "{}";

    BOOST_CHECK_EQUAL("00000000000000006dcd3132e5df480a89a891984bca0a090c696e697464656c6567617465027b7d00e40b5402000000"
                      "09534352000000009b2a645bb92a645b",
                      to_hex(op));
}

SCORUM_TEST_CASE(deserialize_create_budget_operation_from_binary_test)
{
    auto bin = "00000000000000006dcd3132e5df480a89a891984bca0a090c696e697464656c6567617465027b7d00e40b5402000000"
               "09534352000000009b2a645bb92a645b";

    auto op = from_hex<create_budget_operation>(bin);

    BOOST_CHECK_EQUAL(bin, to_hex(op));
}

SCORUM_TEST_CASE(serialize_create_budget_operation_to_json_test)
{
    create_budget_operation op;
    op.uuid = boost::uuids::string_generator()("6DCD3132-E5DF-480A-89A8-91984BCA0A09");
    op.type = scorum::protocol::budget_type::post;
    op.owner = "initdelegate";
    op.balance = ASSET("10.000000000 SCR");
    op.start = fc::time_point_sec::from_iso_string("2018-08-03T10:12:43");
    op.deadline = fc::time_point_sec::from_iso_string("2018-08-03T10:13:13");
    op.json_metadata = "{}";

    auto json_actual = fc::json::to_string(op);
    auto json_expected = R"({
                              "type": "post",
                              "uuid": "6dcd3132-e5df-480a-89a8-91984bca0a09",
                              "owner": "initdelegate",
                              "json_metadata": "{}",
                              "balance": "10.000000000 SCR",
                              "start": "2018-08-03T10:12:43",
                              "deadline": "2018-08-03T10:13:13"
                            })";
    BOOST_CHECK_EQUAL(fc::json::to_string(fc::json::from_string(json_expected)), json_actual);
}

SCORUM_TEST_CASE(deserialize_create_budget_operation_from_json_test)
{
    auto json = R"({
                     "type": "post",
                     "uuid": "6dcd3132-e5df-480a-89a8-91984bca0a09",
                     "owner": "initdelegate",
                     "json_metadata": "{}",
                     "balance": "10.000000000 SCR",
                     "start": "2018-08-03T10:12:43",
                     "deadline": "2018-08-03T10:13:13"
                   })";
    auto op = fc::json::from_string(json).as<create_budget_operation>();

    BOOST_CHECK_EQUAL(fc::json::to_string(fc::json::from_string(json)), fc::json::to_string(op));
}

BOOST_AUTO_TEST_SUITE_END()
}
