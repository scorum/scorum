#include "registration_check_common.hpp"

#include <scorum/protocol/config.hpp>

#include "database_fixture.hpp"

namespace registration_check {

namespace {

genesis_state_type create_registration_genesis_impl(schedule_inputs_type& schedule_input,
                                                    asset& rest_of_supply,
                                                    committee_private_keys_type& committee_private_keys)
{
    genesis_state_type genesis_state;

    genesis_state.registration_committee.emplace_back("alice");
    genesis_state.registration_committee.emplace_back("bob");

    committee_private_keys.clear();
    for (const account_name_type& member : genesis_state.registration_committee)
    {
        fc::ecc::private_key private_key = database_fixture::generate_private_key(member);
        committee_private_keys.insert(committee_private_keys_type::value_type(member, private_key));
        genesis_state.accounts.push_back({ member, "", private_key.get_public_key(), share_type(0), share_type(0) });
    }

    schedule_input.clear();
    schedule_input.reserve(4);

    schedule_input.emplace_back(schedule_input_type{ 1, 2, 100 });
    schedule_input.emplace_back(schedule_input_type{ 2, 2, 75 });
    schedule_input.emplace_back(schedule_input_type{ 3, 1, 50 });
    schedule_input.emplace_back(schedule_input_type{ 4, 3, 25 });

    // half of limit
    genesis_state.registration_maximum_bonus = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
    genesis_state.registration_maximum_bonus.amount /= 2;

    genesis_state.registration_schedule = schedule_input;

    genesis_state.registration_supply
        = schedule_input_total_bonus(schedule_input, genesis_state.registration_maximum_bonus);
    rest_of_supply = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
    genesis_state.registration_supply += rest_of_supply;

    return genesis_state;
}
}

asset schedule_input_total_bonus(const schedule_inputs_type& schedule_input, const asset& maximum_bonus)
{
    asset ret(0, REGISTRATION_BONUS_SYMBOL);
    for (const auto& item : schedule_input)
    {
        share_type stage_amount = maximum_bonus.amount;
        stage_amount *= item.bonus_percent;
        stage_amount /= 100;
        ret += asset(stage_amount * item.users, REGISTRATION_BONUS_SYMBOL);
    }
    return ret;
}

genesis_state_type create_registration_genesis(schedule_inputs_type& schedule_input, asset& rest_of_supply)
{
    committee_private_keys_type committee_private_keys;
    schedule_input.clear();
    return create_registration_genesis_impl(schedule_input, rest_of_supply, committee_private_keys);
}

genesis_state_type create_registration_genesis()
{
    committee_private_keys_type committee_private_keys;
    schedule_inputs_type schedule_input;
    asset rest_of_supply;
    return create_registration_genesis_impl(schedule_input, rest_of_supply, committee_private_keys);
}

genesis_state_type create_registration_genesis(committee_private_keys_type& committee_private_keys)
{
    schedule_inputs_type schedule_input;
    asset rest_of_supply;
    committee_private_keys.clear();
    return create_registration_genesis_impl(schedule_input, rest_of_supply, committee_private_keys);
}

} // namespace registration_check
