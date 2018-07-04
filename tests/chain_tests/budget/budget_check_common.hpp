#pragma once

#include "database_default_integration.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>

#include <scorum/chain/schema/budget_objects.hpp>

namespace budget_check_common {

using namespace database_fixture;

using namespace scorum::chain;
using namespace scorum::protocol;

struct budget_check_fixture : public database_default_integration_fixture
{
    budget_check_fixture();

    void create_budget(const Actor& owner, const budget_type type);
    void create_budget(const Actor& owner, const budget_type type, int balance, int deadline_in_blocks);
    void
    create_budget(const Actor& owner, const budget_type type, int balance, int start_in_blocks, int deadline_in_blocks);
    void create_budget(const Actor& owner,
                       const budget_type type,
                       const asset& balance,
                       const fc::time_point_sec& start,
                       const fc::time_point_sec& deadline);

    const int BUDGET_BALANCE_DEFAULT = 50;
    const int BUDGET_DEADLINE_IN_BLOCKS_DEFAULT = 5;

    post_budget_service_i& post_budget_service;
    banner_budget_service_i& banner_budget_service;

private:
    std::string get_unique_permlink();
};

} // namespace database_fixture
