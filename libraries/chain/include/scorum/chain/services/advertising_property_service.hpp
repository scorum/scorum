#pragma once
#include <scorum/chain/schema/advertising_property_object.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct advertising_property_service_i : public base_service_i<advertising_property_object>
{
};

using dbs_advertising_property = dbs_service_base<advertising_property_service_i>;
}
}