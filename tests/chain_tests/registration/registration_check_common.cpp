#include "registration_check_common.hpp"

#include <scorum/protocol/config.hpp>

#include "database_default_integration.hpp"

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/genesis/initializators/registration_initializator.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include "actoractions.hpp"

namespace database_fixture {

using namespace database_fixture;

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
    , registration_pool_service(db.registration_pool_service())
    , registration_committee_service(db.registration_committee_service())
{
    open_database();

    Actor alice("alice");
    Actor bob("bob");

    _committee[alice.name] = alice;
    _committee[bob.name] = bob;

    for (const auto& item : _committee)
    {
        actor(initdelegate).create_account(Actor(item.second));
    }
}

asset registration_check_fixture::registration_supply()
{
    FC_ASSERT(_registration_supply.amount > 0,
              "Registration data is not initialized. Call create_registration_genesis.");
    return _registration_supply;
}

asset registration_check_fixture::registration_bonus()
{
    return _registration_bonus;
}

asset registration_check_fixture::rest_of_supply()
{
    return registration_bonus();
}

const account_object& registration_check_fixture::committee_member()
{
    return account_service.get_account(_committee.begin()->first);
}

void registration_check_fixture::create_registration_objects(const genesis_state_type& genesis)
{
    generate_blocks(2);

    db_plugin->debug_update(
        [&](database&) {

            genesis::registration_initializator_impl creator;
            genesis::initializator_context ctx(_services, genesis);
            creator.apply(ctx);

            dynamic_global_property_service_i& dgp_service = _services.dynamic_global_property_service();

            dgp_service.update(
                [&](dynamic_global_property_object& gpo) { gpo.total_supply += genesis.registration_supply; });

        },
        get_skip_flags());

    generate_blocks(2);
}

const registration_pool_object& registration_check_fixture::create_pool(const genesis_state_type& genesis_state)
{
    // create sorted items list form genesis unordered data
    using schedule_item_type = registration_pool_object::schedule_item;
    using sorted_type = std::map<uint8_t, schedule_item_type>;
    sorted_type items;
    for (const auto& genesis_item : genesis_state.registration_schedule)
    {
        items.insert(sorted_type::value_type(genesis_item.stage,
                                             schedule_item_type{ genesis_item.users, genesis_item.bonus_percent }));
    }

    return registration_pool_service.create_pool(genesis_state.registration_supply, genesis_state.registration_bonus,
                                                 items);
}

void create_schedule(schedule_inputs_type& schedule)
{
    schedule.clear();
    schedule.reserve(4);

    // Amount of users = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK to force
    // selection the entire limit in the first step (due tests logic)
    schedule.emplace_back(schedule_input_type{ 1, SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK, 100 });
    schedule.emplace_back(schedule_input_type{ 2, 5, 75 });
    schedule.emplace_back(schedule_input_type{ 3, 5, 50 });
    schedule.emplace_back(schedule_input_type{ 4, 8, 25 });
}

genesis_state_type registration_check_fixture::create_registration_genesis()
{
    genesis_state_type genesis_state;

    for (const auto& item : _committee)
    {
        genesis_state.registration_committee.emplace_back(item.second.name);
        genesis_state.accounts.push_back({ item.second.name, item.second.public_key, asset(0, SCORUM_SYMBOL) });
    }

    genesis_state.registration_bonus = registration_bonus();

    create_schedule(genesis_state.registration_schedule);

    _registration_supply
        = schedule_input_total_bonus(genesis_state.registration_schedule, genesis_state.registration_bonus);
    _registration_supply += rest_of_supply();
    genesis_state.registration_supply = _registration_supply;

    return genesis_state;
}

asset registration_check_fixture::registration_supply() const
{
    return _registration_supply;
}
}
