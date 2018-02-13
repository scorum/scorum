#pragma once

#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/chain/services/account.hpp>

#include <fc/crypto/digest.hpp>

#include <vector>
#include <map>

#include "database_trx_integration.hpp"

namespace scorum {
namespace chain {

using schedule_input_type = genesis_state_type::registration_schedule_item;
using schedule_inputs_type = std::vector<schedule_input_type>;
using committee_private_keys_type = std::map<account_name_type, private_key_type>;

asset schedule_input_total_bonus(const schedule_inputs_type& schedule_input, const asset& maximum_bonus);

class registration_objects_fixture : public database_trx_integration_fixture
{
public:
    registration_objects_fixture();

    void create_registration_objects(const genesis_state_type&);

    genesis_state_type create_registration_genesis(schedule_inputs_type& schedule_input, asset& rest_of_supply);
    genesis_state_type create_registration_genesis();
    genesis_state_type create_registration_genesis(committee_private_keys_type& committee_private_keys);

    const account_object& bonus_beneficiary();

private:
    genesis_state_type create_registration_genesis_impl(schedule_inputs_type& schedule_input,
                                                        asset& rest_of_supply,
                                                        committee_private_keys_type& committee_private_keys);

    data_service_factory_i& _services;

public:
    account_service_i& account_service;
};
}
}
