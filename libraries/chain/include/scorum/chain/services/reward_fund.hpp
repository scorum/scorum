#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

namespace scorum {
namespace chain {

struct reward_fund_service_i : public base_service_i<reward_fund_object>
{
};

using dbs_reward_fund = dbs_service_base<reward_fund_service_i>;

} // namespace scorum
} // namespace chain
