#pragma once
#include <scorum/chain/schema/betting_property_object.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct betting_property_service_i : public base_service_i<betting_property_object>
{
};

using dbs_betting_property = dbs_service_base<betting_property_service_i>;
}
}
