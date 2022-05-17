#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/protocol/transaction.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/io/json.hpp>

#include "defines.hpp"
#include "detail.hpp"

using detail::to_hex;

namespace signed_transaction_serialization_tests {

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

SCORUM_TEST_CASE(serialize_empty_signed_transaction_to_json)
{
    signed_transaction trx;
    BOOST_CHECK_EQUAL(
        R"({"ref_block_num":0,"ref_block_prefix":0,"expiration":"1970-01-01T00:00:00","operations":[],"extensions":[],"signatures":[]})",
        fc::json::to_string(trx));
}

SCORUM_TEST_CASE(serialize_signed_transaction_with_transfer_operation_to_json)
{
    signed_transaction trx;
    trx.operations.push_back(op);

    BOOST_CHECK_EQUAL(
        R"({"ref_block_num":0,"ref_block_prefix":0,"expiration":"1970-01-01T00:00:00","operations":[["transfer",{"from":"alice","to":"bob","amount":"0.000000001 SCR","memo":""}]],"extensions":[],"signatures":[]})",
        fc::json::to_string(trx).c_str());
}

SCORUM_TEST_CASE(serialize_empty_signed_transaction_to_raw)
{
    signed_transaction trx;
    BOOST_CHECK_EQUAL("00000000000000000000000000", to_hex(trx));
}

SCORUM_TEST_CASE(serialize_not_empty_signed_transaction_to_raw)
{
    signed_transaction trx;
    trx.ref_block_num = 10;
    trx.ref_block_prefix = 100;
    BOOST_CHECK_EQUAL("0a006400000000000000000000", to_hex(trx));
}

BOOST_AUTO_TEST_CASE(serialize_transfer_to_raw)
{
    BOOST_CHECK_EQUAL("05616c69636503626f620100000000000000095343520000000000", to_hex(op));
}

SCORUM_TEST_CASE(serialize_signed_transaction_with_transfer_operation_to_raw)
{
    signed_transaction trx;
    trx.ref_block_num = 10;
    trx.ref_block_prefix = 100;
    trx.operations.push_back(op);

    BOOST_CHECK_EQUAL("0a006400000000000000010205616c69636503626f6201000000000000000953435200000000000000",
                      to_hex(trx));
}

SCORUM_TEST_CASE(testing_transaction_raw_pack_and_unpack)
{
    signed_transaction trx;
    trx.operations.push_back(op);
    auto packed = fc::raw::pack(trx);
    signed_transaction unpacked = fc::raw::unpack<signed_transaction>(packed);
    unpacked.validate();
    BOOST_CHECK(trx.digest() == unpacked.digest());
}

SCORUM_TEST_CASE(testing_transaction_json_pack_and_unpack)
{
    fc::variant test(op.amount);
    asset tmp = test.as<asset>();
    BOOST_REQUIRE(tmp == op.amount);

    signed_transaction trx;
    trx.operations.push_back(op);
    fc::variant packed(trx);
    signed_transaction unpacked = packed.as<signed_transaction>();
    unpacked.validate();
    BOOST_CHECK(trx.digest() == unpacked.digest());
}

SCORUM_TEST_CASE(wincase_update_transaction_serialization_test)
{
    scorum::protocol::transaction trx;
    trx.ref_block_num = 10483;
    trx.ref_block_prefix = 285062829;
    trx.expiration = fc::time_point_sec::from_iso_string("2018-10-18T09:11:09");

    scorum::protocol::witness_update_operation op;
    op.owner = "leonarda";
    op.url = "leonarda.com";
    op.block_signing_key = scorum::protocol::public_key_type("SCR7zPNg5nAsJjP9gvMfQ4UnAwDwf91WPYC8KFzobtMuQ52ns1D6T");
    op.proposed_chain_props.account_creation_fee = asset::from_string("0.100000000 SCR");
    op.proposed_chain_props.maximum_block_size = 131072;

    trx.operations.push_back(op);

    BOOST_CHECK_EQUAL("f328adb6fd102d4ec85b0109086c656f6e617264610c6c656f6e617264612e636f6d03987a5a967458c114c15091198c"
                      "06a822f54b494ea486204551a53f85effa314200e1f5050000000009534352000000000000020000",
                      to_hex(trx));
}

SCORUM_TEST_CASE(key_authority_map_marshall_test)
{
    using scorum::protocol::public_key_type;
    using scorum::protocol::authority_weight_type;
    using scorum::protocol::authority;

    auto key = public_key_type("SCR7zPNg5nAsJjP9gvMfQ4UnAwDwf91WPYC8KFzobtMuQ52ns1D6T");
    auto key2 = public_key_type("SCR77iaW1ccqgGE9VZyCedTHpwmJ14DA6J4htXjyY8CWd44UM6rav");

    authority::key_authority_map authorities;
    authorities.insert(std::pair<public_key_type, authority_weight_type>(key, 1));
    authorities.insert(std::pair<public_key_type, authority_weight_type>(key2, 2));

    BOOST_CHECK_EQUAL("0203256d923feba40de45ef9090bac0b86fbab236681b04f944ecaf333eb25b52a59020003987a5a967458c114c15091"
                      "198c06a822f54b494ea486204551a53f85effa31420100",
                      to_hex(authorities));
}

SCORUM_TEST_CASE(account_authority_map_marshall_test)
{
    scorum::protocol::account_authority_map authorities;
    authorities.insert(std::pair<scorum::protocol::account_name_type , scorum::protocol::authority_weight_type>("alice", 1));
    BOOST_CHECK_EQUAL("0105616c6963650100", to_hex(authorities));
}

SCORUM_TEST_CASE(account_create_by_committee_operation_marshall_test)
{
    auto owner = scorum::protocol::public_key_type("SCR7zPNg5nAsJjP9gvMfQ4UnAwDwf91WPYC8KFzobtMuQ52ns1D6T");
    auto active = scorum::protocol::public_key_type("SCR7SHdKpjpWyfj32tGQBeijFfokmCARjKSBynqDwN1ZAbQRW5rWa");
    auto posting = scorum::protocol::public_key_type("SCR5jPZF7PMgTpLqkdfpMu8kXea8Gio6E646aYpTgcjr9qMLrAgnL");

    scorum::protocol::account_create_by_committee_operation op;
    op.creator = "alice";
    op.new_account_name = "bob";
    op.owner = scorum::protocol::authority(1, owner, 1);
    op.active = scorum::protocol::authority(1, active, 1);
    op.posting = scorum::protocol::authority(1, posting, 1);
//    op.memo_key = scorum::protocol::public_key_type("");
    op.json_metadata = "";

    BOOST_CHECK_EQUAL("05616c69636503626f6201000000000103987a5a967458c114c15091198c06a822f54b494ea486204551a53f85effa31420100010000000001034f97d09e6de4778300ed176403e5b4298bfd62f0fb6edb4a6072e7214318d9030100010000000001026f0896f24d94252c351715bfe6052bbf9ea820e805bd47c2496c626d3467da5d010000000000000000000000000000000000000000000000000000000000000000000000", to_hex(op));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace signed_transaction_serialization_tests
