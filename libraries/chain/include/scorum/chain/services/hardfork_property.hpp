#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/hardfork.hpp>

namespace scorum {
namespace chain {

struct hardfork_property_service_i : public base_service_i<hardfork_property_object>
{
};

using dbs_hardfork_property = dbs_service_base<hardfork_property_service_i>;

} // namespace chain
} // namespace scorum
