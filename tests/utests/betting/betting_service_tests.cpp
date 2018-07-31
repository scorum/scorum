#include <boost/test/unit_test.hpp>

#include "service_wrappers.hpp"

#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/services/betting_service.hpp>

namespace betting_service_tests {

using namespace scorum::chain;
using namespace service_wrappers;

class betting_service : public dbs_betting
{
public:
    betting_service(data_service_factory_i* pdbs_factory)
        : dbs_betting(*(database*)pdbs_factory)
    {
    }
};

struct betting_service_fixture : public shared_memory_fixture
{
    MockRepository mocks;

    data_service_factory_i* dbs_services = mocks.Mock<data_service_factory_i>();

    const account_name_type moderator = "smit";

    betting_service_fixture()
        : betting_property(*this, mocks, [&](betting_property_object& bp) { bp.moderator = moderator; })
        , bet(*this, mocks)
        , pending_bet(*this, mocks)
        , matched_bet(*this, mocks)
        , dgp_bet(*this, mocks)
    {
        mocks.OnCall(dbs_services, data_service_factory_i::betting_property_service)
            .ReturnByRef(betting_property.service());
        mocks.OnCall(dbs_services, data_service_factory_i::bet_service).ReturnByRef(bet.service());
        mocks.OnCall(dbs_services, data_service_factory_i::pending_bet_service).ReturnByRef(pending_bet.service());
        mocks.OnCall(dbs_services, data_service_factory_i::matched_bet_service).ReturnByRef(matched_bet.service());
        mocks.OnCall(dbs_services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(dgp_bet.service());
    }

protected:
    service_base_wrapper<betting_property_service_i> betting_property;
    service_base_wrapper<bet_service_i> bet;
    service_base_wrapper<pending_bet_service_i> pending_bet;
    service_base_wrapper<matched_bet_service_i> matched_bet;
    service_base_wrapper<dynamic_global_property_service_i> dgp_bet;
};

BOOST_FIXTURE_TEST_CASE(budget_service_is_betting_moderator_check, betting_service_fixture)
{
    betting_service service(dbs_services);

    BOOST_CHECK(!service.is_betting_moderator("jack"));
    BOOST_CHECK(service.is_betting_moderator(moderator));
}
}
