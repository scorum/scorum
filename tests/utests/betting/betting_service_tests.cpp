#include <boost/test/unit_test.hpp>

#include "service_wrappers.hpp"

#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/betting_service.hpp>
#include <scorum/chain/dba/db_accessor_mock.hpp>
#include <scorum/chain/dba/db_accessor_i.hpp>
#include <scorum/chain/dba/db_accessor_factory.hpp>

namespace betting_service_tests {

using namespace scorum::chain;
using namespace service_wrappers;
using namespace scorum::chain::dba;

class betting_service : public dbs_betting
{
public:
    betting_service(db_accessor_factory* dba_factory)
        : dbs_betting(*(database*)dba_factory)
    {
    }
};

struct betting_service_fixture : public shared_memory_fixture
{
    MockRepository mocks;

    db_accessor_factory* dba_factory = mocks.Mock<db_accessor_factory>();

    const account_name_type moderator = "smit";

    betting_service_fixture()
        : betting_prop_dba_i(db_accessor_mock<betting_property_object>{})
        , betting_prop_dba(betting_prop_dba_i.get_accessor_inst<db_accessor_mock<betting_property_object>>())
        , obj(create_object<betting_property_object>(shm, [&](betting_property_object& o) { o.moderator = moderator; }))
    {
        mocks.OnCallFunc(get_db_accessor<betting_property_object>).ReturnByRef(betting_prop_dba_i);
        betting_prop_dba.mock(&db_accessor_mock<betting_property_object>::get,
                              [&]() -> decltype(auto) { return (obj); });
    }

protected:
    db_accessor_i<betting_property_object> betting_prop_dba_i;
    db_accessor_mock<betting_property_object>& betting_prop_dba;

    betting_property_object obj;
};

BOOST_FIXTURE_TEST_CASE(budget_service_is_betting_moderator_check, betting_service_fixture)
{
    betting_service service(dba_factory);

    BOOST_CHECK(!service.is_betting_moderator("jack"));
    BOOST_CHECK(service.is_betting_moderator(moderator));
}
}
