#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/protocol/transaction.hpp>

#include <fc/io/json.hpp>

#include "defines.hpp"
#include "utils.hpp"

using scorum::protocol::asset;
using scorum::protocol::signed_transaction;
using scorum::protocol::transfer_operation;

class fixture
{
public:
    fixture()
    {
        op.from = "alice";
        op.to = "bob";
        op.amount = asset(1, SCORUM_SYMBOL);
        op.memo = "";
    }

    transfer_operation op;
};

BOOST_FIXTURE_TEST_SUITE(signed_transaction_serialization_tests, fixture)

SCORUM_TEST_CASE(serialize_transfer_to_json)
{
    BOOST_CHECK_EQUAL(R"({"from":"alice","to":"bob","amount":"0.000000001 SCR","memo":""})",
                      fc::json::to_string(op).c_str());
}

// SCORUM_TEST_CASE(serialize_empty_signed_transaction_to_json)
//{
//    signed_transaction trx;
//    BOOST_CHECK_EQUAL(
//        R"({"ref_block_num":0,"ref_block_prefix":0,"expiration":"1970-01-01T00:00:00","operations":[],"extensions":[],"signatures":[]})",
//        fc::json::to_string(trx));
//}

// SCORUM_TEST_CASE(serialize_signed_transaction_with_transfer_operation_to_json)
//{
//    signed_transaction trx;
//    trx.operations.push_back(op);

//    BOOST_CHECK_EQUAL(
//        R"({"ref_block_num":0,"ref_block_prefix":0,"expiration":"1970-01-01T00:00:00","operations":[["transfer",{"from":"alice","to":"bob","amount":"0.000000001
//        SCR","memo":""}]],"extensions":[],"signatures":[]})",
//        fc::json::to_string(trx).c_str());
//}

// SCORUM_TEST_CASE(serialize_empty_signed_transaction_to_raw)
//{
//    signed_transaction trx;
//    BOOST_CHECK_EQUAL("00000000000000000000000000", utils::to_hex(trx));
//}

// SCORUM_TEST_CASE(serialize_not_empty_signed_transaction_to_raw)
//{
//    signed_transaction trx;
//    trx.ref_block_num = 10;
//    trx.ref_block_prefix = 100;
//    BOOST_CHECK_EQUAL("0a006400000000000000000000", utils::to_hex(trx));
//}

// BOOST_AUTO_TEST_CASE(serialize_transfer_to_raw)
//{
//    BOOST_CHECK_EQUAL("05616c69636503626f620100000000000000095343520000000000", utils::to_hex(op));
//}

// SCORUM_TEST_CASE(serialize_signed_transaction_with_transfer_operation_to_raw)
//{
//    signed_transaction trx;
//    trx.ref_block_num = 10;
//    trx.ref_block_prefix = 100;
//    trx.operations.push_back(op);

//    BOOST_CHECK_EQUAL("0a006400000000000000010205616c69636503626f6201000000000000000953435200000000000000",
//                      utils::to_hex(trx));
//}

// SCORUM_TEST_CASE(testing_transaction_raw_pack_and_unpack)
//{
//    signed_transaction trx;
//    trx.operations.push_back(op);
//    auto packed = fc::raw::pack(trx);
//    signed_transaction unpacked = fc::raw::unpack<signed_transaction>(packed);
//    unpacked.validate();
//    BOOST_CHECK(trx.digest() == unpacked.digest());
//}

// SCORUM_TEST_CASE(testing_transaction_json_pack_and_unpack)
//{
//    fc::variant test(op.amount);
//    asset tmp = test.as<asset>();
//    BOOST_REQUIRE(tmp == op.amount);

//    signed_transaction trx;
//    trx.operations.push_back(op);
//    fc::variant packed(trx);
//    signed_transaction unpacked = packed.as<signed_transaction>();
//    unpacked.validate();
//    BOOST_CHECK(trx.digest() == unpacked.digest());
//}

BOOST_AUTO_TEST_SUITE_END()
