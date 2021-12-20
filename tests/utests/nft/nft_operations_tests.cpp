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

BOOST_AUTO_TEST_SUITE_END()

} // namespace