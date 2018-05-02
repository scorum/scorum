#pragma once

#include <scorum/chain/services/service_base.hpp>

#include <scorum/chain/schema/reward_objects.hpp>

namespace scorum {
namespace chain {

struct reward_fund_scr_service_i : public base_service_i<reward_fund_scr_object>
{
};

using dbs_reward_fund_scr = dbs_service_base<reward_fund_scr_service_i>;

struct reward_fund_sp_service_i : public base_service_i<reward_fund_sp_object>
{
};

using dbs_reward_fund_sp = dbs_service_base<reward_fund_sp_service_i>;

struct fifa_world_cup_2018_bounty_reward_fund_service_i
    : public base_service_i<fifa_world_cup_2018_bounty_reward_fund_object>
{
};

using dbs_fifa_world_cup_2018_bounty_reward_fund = dbs_service_base<fifa_world_cup_2018_bounty_reward_fund_service_i>;

} // namespace scorum
} // namespace chain
