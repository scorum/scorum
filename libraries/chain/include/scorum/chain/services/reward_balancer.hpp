#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/reward_balancer_objects.hpp>

namespace scorum {
namespace chain {

struct content_reward_scr_service_i : public base_service_i<content_reward_balancer_scr_object>
{
};
struct voters_reward_scr_service_i : public base_service_i<voters_reward_balancer_scr_object>
{
};
struct voters_reward_sp_service_i : public base_service_i<voters_reward_balancer_sp_object>
{
};

using dbs_content_reward_scr = dbs_service_base<content_reward_scr_service_i>;
using dbs_voters_reward_scr = dbs_service_base<voters_reward_scr_service_i>;
using dbs_voters_reward_sp = dbs_service_base<voters_reward_sp_service_i>;

} // namespace chain
} // namespace scorum
