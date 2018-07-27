#pragma once
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct bet_service_i : public base_service_i<bet_object>
{
};

using dbs_bet = dbs_service_base<bet_service_i>;
}
}
