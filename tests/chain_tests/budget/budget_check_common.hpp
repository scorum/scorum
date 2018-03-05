#pragma once

#include "database_default_integration.hpp"

namespace database_fixture {

struct budget_check_fixture : public database_default_integration_fixture
{
    budget_check_fixture();

    fc::time_point_sec default_deadline;
    const int BLOCK_LIMIT_DEFAULT = 5;

private:
    static bool _time_printed;
};

} // namespace database_fixture
