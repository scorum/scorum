#pragma once

#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>

#include <fc/crypto/digest.hpp>

#include <vector>
#include <map>

#include "database_trx_integration.hpp"
#include "actor.hpp"

namespace registration_fixtures {

using namespace scorum::chain;

using schedule_input_type = genesis_state_type::registration_schedule_item;
using schedule_inputs_type = std::vector<schedule_input_type>;
using committee_private_keys_type = std::map<account_name_type, private_key_type>;

asset schedule_input_total_bonus(const schedule_inputs_type& schedule_input, const asset& maximum_bonus);

class registration_check_fixture : public database_trx_integration_fixture
{
public:
    registration_check_fixture();

    asset registration_supply();
    asset registration_bonus();
    asset rest_of_supply();
    const account_object& committee_member();

    void create_registration_objects(const genesis_state_type&);
    const registration_pool_object& create_pool(const genesis_state_type& genesis_state);

    genesis_state_type create_registration_genesis(schedule_inputs_type& schedule_input);
    genesis_state_type create_registration_genesis();
    genesis_state_type create_registration_genesis(committee_private_keys_type& committee_private_keys);

private:
    genesis_state_type create_registration_genesis_impl(schedule_inputs_type& schedule_input,
                                                        committee_private_keys_type& committee_private_keys);

    data_service_factory_i& _services;
    asset _registration_supply = asset(0, SCORUM_SYMBOL);
    const asset _registration_bonus = ASSET_SCR(100);
    Actors _committee;

public:
    account_service_i& account_service;
    registration_pool_service_i& registration_pool_service;
    registration_committee_service_i& registration_committee_service;
};
}
