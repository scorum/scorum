#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/account_objects.hpp>

namespace scorum {
namespace chain {

struct account_registration_bonus_service_i : public base_service_i<account_registration_bonus_object>
{
};

using dbs_account_registration_bonus = dbs_service_base<account_registration_bonus_service_i>;
}
}
