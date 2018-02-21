#include <boost/test/unit_test.hpp>

#include <scorum/chain/database/database.hpp>

#include <exception>

#include "database_default_integration.hpp"

namespace scorum {
namespace chain {

database_default_integration_fixture::~database_default_integration_fixture()
{
    try
    {
        // If we're unwinding due to an exception, don't do any more checks.
        // This way, boost test's last checkpoint tells us approximately where the error was.
        if (!std::uncaught_exception())
        {
            BOOST_CHECK(db.get_node_properties().skip_flags == database::skip_nothing);
        }
    }
    FC_CAPTURE_AND_RETHROW()
}

} // namespace chain
} // namespace scorum
