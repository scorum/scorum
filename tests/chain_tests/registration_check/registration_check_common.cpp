#include "registration_check_common.hpp"

#include <scorum/protocol/config.hpp>

#include "database_default_integration.hpp"

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/genesis/initializators/registration_initializator.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#define MEMBER_BONUS_BENEFICIARY alice
#define NEXT_MEMBER bob

namespace scorum {
namespace chain {

asset schedule_input_total_bonus(const schedule_inputs_type& schedule_input, const asset& maximum_bonus)
{
    asset ret(0, SCORUM_SYMBOL);
    for (const auto& item : schedule_input)
    {
        share_type stage_amount = maximum_bonus.amount;
        stage_amount *= item.bonus_percent;
        stage_amount /= 100;
        ret += asset(stage_amount * item.users, SCORUM_SYMBOL);
    }
    return ret;
}

registration_check_fixture::registration_check_fixture()
    : _services(static_cast<data_service_factory_i&>(db))
    , account_service(db.account_service())
{
    open_database();

    ACTORS((MEMBER_BONUS_BENEFICIARY)(NEXT_MEMBER));
}

void registration_check_fixture::create_registration_objects(const genesis_state_type& genesis)
{
    generate_blocks(5);

    db_plugin->debug_update(
        [&](database&) {

            genesis::registration_initializator_impl creator;
            genesis::initializator_context ctx(_services, genesis);
            creator.apply(ctx);

            dynamic_global_property_service_i& dgp_service = _services.dynamic_global_property_service();

            dgp_service.update(
                [&](dynamic_global_property_object& gpo) { gpo.total_supply += genesis.registration_supply; });

        },
        default_skip);

    generate_blocks(5);
}

genesis_state_type registration_check_fixture::create_registration_genesis(schedule_inputs_type& schedule_input,
                                                                           asset& rest_of_supply)
{
    committee_private_keys_type committee_private_keys;
    schedule_input.clear();
    return create_registration_genesis_impl(schedule_input, rest_of_supply, committee_private_keys);
}

genesis_state_type registration_check_fixture::create_registration_genesis()
{
    committee_private_keys_type committee_private_keys;
    schedule_inputs_type schedule_input;
    asset rest_of_supply;
    return create_registration_genesis_impl(schedule_input, rest_of_supply, committee_private_keys);
}

genesis_state_type
registration_check_fixture::create_registration_genesis(committee_private_keys_type& committee_private_keys)
{
    schedule_inputs_type schedule_input;
    asset rest_of_supply;
    committee_private_keys.clear();
    return create_registration_genesis_impl(schedule_input, rest_of_supply, committee_private_keys);
}

const account_object& registration_check_fixture::bonus_beneficiary()
{
    return account_service.get_account(BOOST_PP_STRINGIZE(MEMBER_BONUS_BENEFICIARY));
}

genesis_state_type registration_check_fixture::create_registration_genesis_impl(
    schedule_inputs_type& schedule_input, asset& rest_of_supply, committee_private_keys_type& committee_private_keys)
{
    genesis_state_type genesis_state;

    genesis_state.registration_committee.emplace_back(BOOST_PP_STRINGIZE(MEMBER_BONUS_BENEFICIARY));
    genesis_state.registration_committee.emplace_back(BOOST_PP_STRINGIZE(NEXT_MEMBER));

    committee_private_keys.clear();
    for (const account_name_type& member : genesis_state.registration_committee)
    {
        fc::ecc::private_key private_key = database_integration_fixture::generate_private_key(member);
        committee_private_keys.insert(committee_private_keys_type::value_type(member, private_key));
        genesis_state.accounts.push_back({ member, "", private_key.get_public_key(), asset(0, SCORUM_SYMBOL) });
    }

    schedule_input.clear();
    schedule_input.reserve(4);

    schedule_input.emplace_back(schedule_input_type{ 1, 2, 100 });
    schedule_input.emplace_back(schedule_input_type{ 2, 2, 75 });
    schedule_input.emplace_back(schedule_input_type{ 3, 1, 50 });
    schedule_input.emplace_back(schedule_input_type{ 4, 3, 25 });

    // half of limit
    genesis_state.registration_bonus = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
    genesis_state.registration_bonus.amount /= 2;

    genesis_state.registration_schedule = schedule_input;

    genesis_state.registration_supply = schedule_input_total_bonus(schedule_input, genesis_state.registration_bonus);
    rest_of_supply = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
    genesis_state.registration_supply += rest_of_supply;

    return genesis_state;
}
}
}
