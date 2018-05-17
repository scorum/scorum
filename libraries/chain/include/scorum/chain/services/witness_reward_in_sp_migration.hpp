#pragma once

#include <scorum/chain/services/service_base.hpp>

#include <scorum/chain/schema/witness_objects.hpp>

namespace scorum {
namespace chain {

struct witness_reward_in_sp_migration_service_i : public base_service_i<witness_reward_in_sp_migration_object>
{
};

using dbs_witness_reward_in_sp_migration = dbs_service_base<witness_reward_in_sp_migration_service_i>;

} // namespace scorum
} // namespace chain
