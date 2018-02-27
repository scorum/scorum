#include <boost/test/unit_test.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <scorum/chain/services/registration_pool.hpp>

#include "registration_check_common.hpp"

#include "genesis.hpp"

using namespace scorum::chain;
using namespace scorum::protocol;

class registration_service_check_fixture : public registration_check_fixture
{
public:
    registration_service_check_fixture()
        : registration_pool_service(db.registration_pool_service())
    {
    }

    const asset registration_bonus = ASSET_SCR(100);

    registration_pool_service_i& registration_pool_service;

    const std::string genesis_invalid_schedule_users_str = R"json(
    {
            "registration_schedule": [
            {
                    "stage": 1,
                    "users": 0,
                    "bonus_percent": 100
            }]
    })json";

    const std::string genesis_invalid_schedule_bonus_percent_l_str = R"json(
    {
            "registration_schedule": [
            {
                    "stage": 1,
                    "users": 1,
                    "bonus_percent": -10
            }]
    })json";

    const std::string genesis_invalid_schedule_bonus_percent_h_str = R"json(
    {
            "registration_schedule": [
            {
                    "stage": 1,
                    "users": 1,
                    "bonus_percent": 110
            }]
    })json";

    const registration_pool_object& create_pool(const genesis_state_type& genesis_state)
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

        return registration_pool_service.create_pool(genesis_state.registration_supply,
                                                     genesis_state.registration_bonus, items);
    }
};

BOOST_FIXTURE_TEST_SUITE(registration_service_genesis_check, registration_service_check_fixture)

SCORUM_TEST_CASE(create_invalid_genesis_state_amount_check)
{
    genesis_state_type invalid_genesis_state = genesis_state;
    invalid_genesis_state.registration_supply = asset(0, SCORUM_SYMBOL);

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);

    invalid_genesis_state = genesis_state;
    invalid_genesis_state.registration_bonus = asset(0, SCORUM_SYMBOL);

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);

    invalid_genesis_state = genesis_state;
    invalid_genesis_state.registration_schedule.clear();

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);
}

SCORUM_TEST_CASE(create_invalid_genesis_schedule_schedule_check)
{
    genesis_state_type invalid_genesis_state;

    invalid_genesis_state = fc::json::from_string(genesis_invalid_schedule_users_str).as<genesis_state_type>();
    invalid_genesis_state.registration_bonus = registration_bonus;
    invalid_genesis_state.registration_supply = registration_bonus;

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);

    invalid_genesis_state
        = fc::json::from_string(genesis_invalid_schedule_bonus_percent_l_str).as<genesis_state_type>();
    invalid_genesis_state.registration_bonus = registration_bonus;
    invalid_genesis_state.registration_supply = registration_bonus;

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);

    invalid_genesis_state
        = fc::json::from_string(genesis_invalid_schedule_bonus_percent_h_str).as<genesis_state_type>();
    invalid_genesis_state.registration_bonus = registration_bonus;
    invalid_genesis_state.registration_supply = registration_bonus;

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

class registration_service_create_check_fixture : public registration_service_check_fixture
{
public:
    registration_service_create_check_fixture()
    {
        open_database();
        Actor alice("alice");
        Actor bob("bob");

        genesis_state = database_integration_fixture::default_genesis_state()
                            .registration_supply(registration_bonus * 100)
                            .registration_bonus(registration_bonus)
                            .registration_schedule(schedule_input_type{ 1, 10, 100 }, schedule_input_type{ 2, 5, 75 },
                                                   schedule_input_type{ 3, 5, 50 }, schedule_input_type{ 4, 8, 25 })
                            .committee(alice, bob)
                            .generate();

        schedule_input = genesis_state.registration_schedule;
        create_pool(genesis_state);
    }

    schedule_inputs_type schedule_input;
};

BOOST_FIXTURE_TEST_SUITE(registration_service_create_check, registration_service_create_check_fixture)

SCORUM_TEST_CASE(create_check)
{
    const registration_pool_object& pool = registration_pool_service.get();

    BOOST_CHECK_EQUAL(pool.balance, genesis_state.registration_supply);
    BOOST_CHECK_EQUAL(pool.maximum_bonus, genesis_state.registration_bonus);
    BOOST_CHECK_EQUAL(pool.already_allocated_count, uint64_t(0));

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    std::size_t ci = 0;
    for (const auto& item : pool.schedule_items)
    {
        BOOST_CHECK_EQUAL(item.users, schedule_input[ci].users);
        BOOST_CHECK_EQUAL(item.bonus_percent, schedule_input[ci].bonus_percent);

        ++ci;
    }
}

SCORUM_TEST_CASE(create_double_check)
{
    BOOST_REQUIRE_THROW(create_pool(genesis_state), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()
