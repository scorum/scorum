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

} // namespace scorum
} // namespace chain
