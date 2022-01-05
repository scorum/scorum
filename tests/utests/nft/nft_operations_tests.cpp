#include <boost/test/unit_test.hpp>

#include <scorum/chain/evaluators/increase_nft_power_evaluator.hpp>

#include <defines.hpp>
#include "detail.hpp"

namespace {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

struct nft_operation_fixture
{
};

BOOST_FIXTURE_TEST_SUITE(nft_operation_tests, nft_operation_fixture)

SCORUM_TEST_CASE(create_nft)
{
    create_nft_operation op;
    op.uuid = gen_uuid("nft");
    op.owner = "user";
    op.power = 1;
    op.json_metadata = "";
    BOOST_CHECK_NO_THROW(op.validate());

    op.json_metadata = R"({"string":"value", "number": 100})";
    BOOST_CHECK_NO_THROW(op.validate());
}

SCORUM_TEST_CASE(create_nft_fail_when_uuid_is_nil)
{
    create_nft_operation op;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(uuid must not be nil)")
}

SCORUM_TEST_CASE(create_nft_fail_when_account_name_is_not_set)
{
    create_nft_operation op;
    op.uuid = gen_uuid("nft");
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, R"(Account name  is invalid)")
}

SCORUM_TEST_CASE(create_nft_fail_when_power_is_zero)
{
    create_nft_operation op;
    op.uuid = gen_uuid("nft");
    op.owner = "user";
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, R"(Cannot create nft with zero or negative power)")
}

SCORUM_TEST_CASE(create_nft_fail_when_meta_is_not_json)
{
    create_nft_operation op;
    op.uuid = gen_uuid("nft");
    op.owner = "user";
    op.json_metadata = "meta";
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(Unexpected char '109' in "meta")")
}

SCORUM_TEST_CASE(update_nft_meta)
{
    update_nft_meta_operation op;
    op.uuid = gen_uuid("nft");
    op.moderator = SCORUM_NFT_MODERATOR;
    op.json_metadata = "";
    BOOST_CHECK_NO_THROW(op.validate());

    op.json_metadata = R"({"string":"value", "number": 100})";
    BOOST_CHECK_NO_THROW(op.validate());
}

SCORUM_TEST_CASE(update_nft_meta_fail_when_uuid_is_nil)
{
    update_nft_meta_operation op;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(uuid must not be nil)")
}

SCORUM_TEST_CASE(update_nft_meta_fail_when_moderator_name_is_not_set)
{
    update_nft_meta_operation op;
    op.uuid = gen_uuid("nft");
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(invalid moderator account)")

    op.moderator = "moderator";
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(invalid moderator account)")
}

SCORUM_TEST_CASE(update_nft_meta_fail_when_json_meta_invalid)
{
    update_nft_meta_operation op;
    op.uuid = gen_uuid("nft");
    op.moderator = SCORUM_NFT_MODERATOR;
    op.json_metadata = "abc";
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(Unexpected char '97' in "abc")")
}

SCORUM_TEST_CASE(increase_nft_power)
{
    increase_nft_power_operation op;
    op.uuid = gen_uuid("nft");
    op.moderator = SCORUM_NFT_MODERATOR;
    op.power = 1;
    BOOST_CHECK_NO_THROW(op.validate());
}

SCORUM_TEST_CASE(increase_nft_power_fail_when_uuid_is_nil)
{
    increase_nft_power_operation op;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(uuid must not be nil)")
}

SCORUM_TEST_CASE(increase_nft_power_fail_when_moderator_is_not_set)
{
    increase_nft_power_operation op;
    op.uuid = gen_uuid("nft");
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(invalid moderator account)")

    op.moderator = "moderator";
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(invalid moderator account)")
}

SCORUM_TEST_CASE(increase_nft_power_fail_when_power_is_zero)
{
    increase_nft_power_operation op;
    op.uuid = gen_uuid("nft");
    op.moderator = SCORUM_NFT_MODERATOR;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(power should be greater than zero)")
}

SCORUM_TEST_CASE(increase_nft_power_fail_when_power_is_negative)
{
    increase_nft_power_operation op;
    op.uuid = gen_uuid("nft");
    op.moderator = SCORUM_NFT_MODERATOR;
    op.power = -1;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(power should be greater than zero)")
}

SCORUM_TEST_CASE(create_game_round_operation_fail_uuid_is_nil)
{
    create_game_round_operation op;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(uuid must not be nil)")
}

SCORUM_TEST_CASE(create_game_round_operation_fail_when_verification_key_size_is_incorrect)
{
    create_game_round_operation op;
    op.uuid = gen_uuid("nft");
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(verification_key should have 64 symbols length)")
}

SCORUM_TEST_CASE(create_game_round_operation_fail_when_seed_size_is_incorrect)
{
    create_game_round_operation op;
    op.uuid = gen_uuid("nft");
    op.verification_key = "038b2fbf4e4f066f309991b9c30cb8f887853e54c76dc705f5ece736ead6c856";
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(seed should have 64 symbols length)")
}

SCORUM_TEST_CASE(create_game_round)
{
    create_game_round_operation op;
    op.uuid = gen_uuid("nft");
    op.verification_key = "038b2fbf4e4f066f309991b9c30cb8f887853e54c76dc705f5ece736ead6c856";
    op.seed = "052d8bec6d55f8c489e837c19fa372e00e3433ebfe0068af658e36b0bd1eb722";
    BOOST_CHECK_NO_THROW(op.validate())
}

SCORUM_TEST_CASE(game_round_result_operation_fail_uuid_is_nil)
{
    game_round_result_operation op;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(uuid must not be nil)")
}

SCORUM_TEST_CASE(game_round_result_operation_fail_when_verification_key_size_is_incorrect)
{
    game_round_result_operation op;
    op.uuid = gen_uuid("nft");
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(proof should have 160 symbols length)")
}

SCORUM_TEST_CASE(game_round_result_operation_fail_when_seed_size_is_incorrect)
{
    game_round_result_operation op;
    op.uuid = gen_uuid("nft");
    op.proof = "638f675cd4313ae84aede4940b7691acd904dec141e444187dcec59f2a25a7a4ef5aa2fe3f88cf235c0d63aa6935bef69d5b70caca0d9b4028f75121d030f80a5a4bcf97b36a868ea9a4c2aaa9013200";
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(vrf should have 128 symbols length)")
}

SCORUM_TEST_CASE(game_round_result_operation_fail_when_result_is_incorrect)
{
    game_round_result_operation op;
    op.uuid = gen_uuid("nft");
    op.proof = "638f675cd4313ae84aede4940b7691acd904dec141e444187dcec59f2a25a7a4ef5aa2fe3f88cf235c0d63aa6935bef69d5b70caca0d9b4028f75121d030f80a5a4bcf97b36a868ea9a4c2aaa9013200";
    op.vrf = "6a196a14e4f9fce66112b1b7ac98f2bcd73352b918d298e0b9f894519a65202dba03ddaa5183190bf2b5cd551f9ef14c8d8b02cf15d0188bbc9bcc6a80d7f91c";
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(result should be greater or equal 100)")
    op.result = 1;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, R"(result should be greater or equal 100)")
}

SCORUM_TEST_CASE(game_round_result)
{
    game_round_result_operation op;
    op.uuid = gen_uuid("nft");
    op.proof = "638f675cd4313ae84aede4940b7691acd904dec141e444187dcec59f2a25a7a4ef5aa2fe3f88cf235c0d63aa6935bef69d5b70caca0d9b4028f75121d030f80a5a4bcf97b36a868ea9a4c2aaa9013200";
    op.vrf = "6a196a14e4f9fce66112b1b7ac98f2bcd73352b918d298e0b9f894519a65202dba03ddaa5183190bf2b5cd551f9ef14c8d8b02cf15d0188bbc9bcc6a80d7f91c";
    op.result = 100;
    BOOST_CHECK_NO_THROW(op.validate())
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace