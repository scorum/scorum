#pragma once

#include "database_default_integration.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/schema/budget_objects.hpp>

#include <scorum/protocol/types.hpp>

namespace budget_check_common {

using namespace database_fixture;

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

struct budget_check_fixture : public database_default_integration_fixture
{
    budget_check_fixture();

    create_budget_operation create_budget(const uuid_type& uuid, const Actor& owner, const budget_type type);
    create_budget_operation create_budget(const uuid_type& uuid,
                                          const Actor& owner,
                                          const budget_type type,
                                          int balance,
                                          uint32_t deadline_blocks_offset);
    create_budget_operation create_budget(const uuid_type& uuid,
                                          const Actor& owner,
                                          const budget_type type,
                                          int balance,
                                          uint32_t start_blocks_offset,
                                          uint32_t deadline_blocks_offset);
    create_budget_operation create_budget(const uuid_type& uuid,
                                          const Actor& owner,
                                          const budget_type type,
                                          const asset& balance,
                                          const fc::time_point_sec& start,
                                          const fc::time_point_sec& deadline);

    static const uint32_t BUDGET_BALANCE_DEFAULT = 50;
    static const uint32_t BUDGET_PERBLOCK_DEFAULT = 10;
    static const uint32_t BUDGET_DEADLINE_IN_BLOCKS_DEFAULT = 5;

    post_budget_service_i& post_budget_service;
    banner_budget_service_i& banner_budget_service;

private:
    std::string get_unique_permlink();
    int permlink_no = 0;
};

} // namespace database_fixture
