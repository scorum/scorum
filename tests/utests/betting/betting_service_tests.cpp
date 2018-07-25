#include <boost/test/unit_test.hpp>

#include "service_wrappers.hpp"

namespace betting_service_tests {

using namespace service_wrappers;

struct services_for_budget_fixture : public shared_memory_fixture
{
    MockRepository mocks;

    fc::time_point_sec head_block_time = fc::time_point_sec::from_iso_string("2018-07-01T00:00:00");

    dynamic_global_property_service_wrapper dgp_service_fixture;

    services_for_budget_fixture()
        : dgp_service_fixture(*this, mocks, [&](dynamic_global_property_object& p) {
            p.time = head_block_time;
            p.head_block_number = 1;
        })
    {
    }
};

// TODO
}
