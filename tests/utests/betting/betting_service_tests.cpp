#include <boost/test/unit_test.hpp>

#include <scorum/chain/betting/betting_service.hpp>

#include "betting_common.hpp"

namespace betting_service_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

struct betting_service_fixture : public betting_common::betting_service_fixture_impl
{
    betting_service_fixture()
        : service(*dbs_services)
    {
    }

    betting_service service;
};

BOOST_FIXTURE_TEST_SUITE(betting_service_tests, betting_service_fixture)

SCORUM_TEST_CASE(is_betting_moderator_check)
{
    BOOST_REQUIRE_NE(moderator, "jack");

    BOOST_CHECK(!service.is_betting_moderator("jack"));
    BOOST_CHECK(service.is_betting_moderator(moderator));
}

BOOST_AUTO_TEST_SUITE_END()
}
