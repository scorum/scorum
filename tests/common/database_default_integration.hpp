#pragma once

#include "database_trx_integration.hpp"

namespace database_fixture {

struct database_default_integration_fixture : public database_trx_integration_fixture
{
    database_default_integration_fixture()
    {
        open_database();
    }

    virtual ~database_default_integration_fixture();
};

} // database_fixture
