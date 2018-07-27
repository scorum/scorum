#pragma once
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct matched_bet_service_i : public base_service_i<matched_bet_object>
{
};

using dbs_matched_bet = dbs_service_base<matched_bet_service_i>;
}
}
