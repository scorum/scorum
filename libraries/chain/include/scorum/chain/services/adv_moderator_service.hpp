#pragma once
#include <scorum/chain/schema/adv_moderator_object.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct adv_moderator_service_i : public base_service_i<adv_moderator_object>
{
};

using dbs_adv_moderator = dbs_service_base<adv_moderator_service_i>;
}
}