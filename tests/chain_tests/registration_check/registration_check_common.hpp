#pragma once

#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/genesis_state.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>

#include <fc/crypto/digest.hpp>

#include <vector>
#include <map>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

namespace registration_check {

using schedule_input_type = genesis_state_type::registration_schedule_item;
using schedule_inputs_type = std::vector<schedule_input_type>;
using committee_private_keys_type = std::map<account_name_type, private_key_type>;

asset schedule_input_total_bonus(const schedule_inputs_type& schedule_input, const asset& maximum_bonus);

genesis_state_type create_registration_genesis(schedule_inputs_type& schedule_input, asset& rest_of_supply);
genesis_state_type create_registration_genesis();
genesis_state_type create_registration_genesis(committee_private_keys_type& committee_private_keys);

} // registration_check
