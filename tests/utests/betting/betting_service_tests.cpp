#include <boost/test/unit_test.hpp>

#include "service_wrappers.hpp"

#include <scorum/chain/services/betting_property.hpp>
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
    {
        mocks.OnCall(dbs_services, data_service_factory_i::betting_property_service)
            .ReturnByRef(betting_property.service());
    }

protected:
    service_base_wrapper<betting_property_service_i> betting_property;
};

BOOST_FIXTURE_TEST_CASE(budget_service_is_betting_moderator_check, betting_service_fixture)
{
    betting_service service(dbs_services);

    BOOST_CHECK(!service.is_betting_moderator("jack"));
    BOOST_CHECK(service.is_betting_moderator(moderator));
}
}
