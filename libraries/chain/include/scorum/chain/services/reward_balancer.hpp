#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/reward_balancer_object.hpp>

namespace scorum {
namespace chain {

struct reward_service_i : public base_service_i<reward_balancer_object>
{
};

using dbs_reward = dbs_service_base<reward_service_i>;

} // namespace chain
} // namespace scorum
