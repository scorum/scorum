#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

namespace scorum {
namespace chain {

struct witness_schedule_service_i : public base_service_i<witness_schedule_object>
{
};

using dbs_witness_schedule = dbs_service_base<witness_schedule_service_i>;

} // namespace chain
} // namespace scorum
