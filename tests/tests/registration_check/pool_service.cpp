#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/scorum_objects.hpp>
#include <scorum/chain/dbs_registration_pool.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>

#include "database_fixture.hpp"

#include <vector>
#include <math.h>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

//
// usage for all registration tests 'chain_test  -t registration_*'
//

namespace {

using schedule_input_type = genesis_state_type::registration_schedule_item;
using schedule_inputs_type = std::vector<schedule_input_type>;

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
    const std::string genesis_str = R"json(
    {
            "accounts": [
            {
                    "name": "alice",
                    "recovery_account": "",
                    "public_key": "SCR1111111111111111111111111111111114T1Anm",
                    "scr_amount": 0,
                    "sp_amount": 0
            },
            {
                    "name": "bob",
                    "recovery_account": "",
                    "public_key": "SCR1111111111111111111111111111111114T1Anm",
                    "scr_amount": 0,
                    "sp_amount": 0
            }],
            "registration_committee": ["alice", "bob"],
    })json";

    genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<genesis_state_type>();

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

class registration_pool_service_check_fixture : public timed_blocks_database_fixture
{
public:
    registration_pool_service_check_fixture()
        : timed_blocks_database_fixture(create_registration_genesis(schedule_input, rest_of_supply))
        , registration_pool_service(db.obtain_service<dbs_registration_pool>())
        , registration_committee_service(db.obtain_service<dbs_registration_committee>())
    {
    }

    static schedule_inputs_type schedule_input;
    static asset rest_of_supply;
    dbs_registration_pool& registration_pool_service;
    dbs_registration_committee& registration_committee_service;

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
                                                     genesis_state.registration_maximum_bonus, items);
    }

    uint64_t get_sin_f(uint64_t pos, uint64_t max_pos, uint64_t max_result)
    {
        BOOST_REQUIRE(max_pos > 0);
        const float fpi = 3.141592653589793f;
        float fpos = pos * fpi;
        fpos /= max_pos;

        float ret = std::sin(fpos);
        ret *= max_result;
        if (pos <= max_pos / 2)
        {
            ret += 0.5f;
        }
        else
        {
            ret -= 0.5f;
        }
        return (uint64_t)floor(ret);
    }

    uint64_t schedule_input_max_pos()
    {
        uint64_t ret = 0;
        for (const auto& item : schedule_input)
        {
            ret += item.users;
        }
        return ret;
    }

    asset schedule_input_total_bonus(const asset& maximum_bonus)
    {
        return ::schedule_input_total_bonus(schedule_input, maximum_bonus);
    }

    int schedule_input_bonus_percent_by_pos(uint64_t pos)
    {
        uint64_t rest = pos;
        auto it = schedule_input.begin();
        for (uint64_t stage = 0; it != schedule_input.end(); ++it, ++stage)
        {
            uint64_t item_users_limit = (*it).users;

            if (rest >= item_users_limit)
            {
                rest -= item_users_limit;
            }
            else
            {
                break;
            }
        }

        if (it != schedule_input.end())
        {
            return (*it).bonus_percent;
        }
        else
        {
            return 0;
        }
    }

    asset schedule_input_predicted_allocate_by_pos(uint64_t pos, asset maximum_bonus)
    {
        share_type ret = (share_type)schedule_input_bonus_percent_by_pos(pos);
        ret *= maximum_bonus.amount;
        ret /= 100;
        return asset(ret, maximum_bonus.symbol);
    }

    asset schedule_input_predicted_limit(uint64_t pass_through_blocks)
    {
        share_type ret = (share_type)pass_through_blocks;
        ret *= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK.amount;
        ret /= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
        return asset(ret, REGISTRATION_BONUS_SYMBOL);
    }

    bool schedule_input_pos_reach_limit(uint64_t& pos,
                                        uint64_t max_pos,
                                        asset predicted_limit,
                                        asset maximum_bonus,
                                        std::function<asset(uint64_t pos)> allocate_cash)
    {
        asset allocated(0, REGISTRATION_BONUS_SYMBOL);
        while (pos < max_pos)
        {
            asset predicted_bonus = schedule_input_predicted_allocate_by_pos(pos, maximum_bonus);
            if (predicted_bonus.amount == share_type(0))
            {
                break;
            }

            if (allocated + predicted_bonus > predicted_limit)
            {
                // limit is reached
                return true;
            }

            allocated += allocate_cash(pos);
            ++pos;
        }
        return false;
    }

    asset schedule_input_vest_all(std::function<asset()> allocate_cash)
    {
        asset total_allocated_bonus(0, REGISTRATION_BONUS_SYMBOL);
        auto it = schedule_input.begin();
        for (uint64_t input_pos = 0, stage = 0; it != schedule_input.end(); ++it, ++stage)
        {
            for (uint64_t register_attempts = 0; register_attempts < (uint64_t)(*it).users;
                 ++register_attempts, ++input_pos)
            {
                total_allocated_bonus += allocate_cash();
            }
        }
        return total_allocated_bonus;
    }
};

schedule_inputs_type registration_pool_service_check_fixture::schedule_input;
asset registration_pool_service_check_fixture::rest_of_supply = asset(0, REGISTRATION_BONUS_SYMBOL);

BOOST_FIXTURE_TEST_SUITE(registration_pool_service_check, registration_pool_service_check_fixture)

SCORUM_TEST_CASE(create_invalid_genesis_state_amount_check)
{
    genesis_state_type invalid_genesis_state = genesis_state;
    invalid_genesis_state.registration_supply = asset(0, REGISTRATION_BONUS_SYMBOL);

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);

    invalid_genesis_state = genesis_state;
    invalid_genesis_state.registration_maximum_bonus = asset(0, REGISTRATION_BONUS_SYMBOL);

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);

    invalid_genesis_state = genesis_state;
    invalid_genesis_state.registration_schedule.clear();

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);
}

SCORUM_TEST_CASE(create_invalid_genesis_schedule_schedule_check)
{
    genesis_state_type invalid_genesis_state;

    invalid_genesis_state = fc::json::from_string(genesis_invalid_schedule_users_str).as<genesis_state_type>();
    invalid_genesis_state.registration_maximum_bonus = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
    invalid_genesis_state.registration_supply = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);

    invalid_genesis_state
        = fc::json::from_string(genesis_invalid_schedule_bonus_percent_l_str).as<genesis_state_type>();
    invalid_genesis_state.registration_maximum_bonus = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
    invalid_genesis_state.registration_supply = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);

    invalid_genesis_state
        = fc::json::from_string(genesis_invalid_schedule_bonus_percent_h_str).as<genesis_state_type>();
    invalid_genesis_state.registration_maximum_bonus = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
    invalid_genesis_state.registration_supply = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;

    BOOST_CHECK_THROW(create_pool(invalid_genesis_state), fc::assert_exception);
}

SCORUM_TEST_CASE(create_check)
{
    const registration_pool_object& pool = registration_pool_service.get_pool();

    BOOST_CHECK_EQUAL(pool.balance, genesis_state.registration_supply);
    BOOST_CHECK_EQUAL(pool.maximum_bonus, genesis_state.registration_maximum_bonus);
    BOOST_CHECK_EQUAL(pool.already_allocated_count, 0);

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

SCORUM_TEST_CASE(sin_f_check)
{
    BOOST_CHECK_EQUAL(get_sin_f(0, 10, 5), 0);
    BOOST_CHECK_EQUAL(get_sin_f(1, 10, 5), 2);
    BOOST_CHECK_EQUAL(get_sin_f(2, 10, 5), 3);
    BOOST_CHECK_EQUAL(get_sin_f(3, 10, 5), 4);
    BOOST_CHECK_EQUAL(get_sin_f(4, 10, 5), 5);
    BOOST_CHECK_EQUAL(get_sin_f(5, 10, 5), 5);
    BOOST_CHECK_EQUAL(get_sin_f(6, 10, 5), 4);
    BOOST_CHECK_EQUAL(get_sin_f(7, 10, 5), 3);
    BOOST_CHECK_EQUAL(get_sin_f(8, 10, 5), 2);
    BOOST_CHECK_EQUAL(get_sin_f(9, 10, 5), 1);

    BOOST_CHECK_EQUAL(get_sin_f(0, 5, 5), 0);
    BOOST_CHECK_EQUAL(get_sin_f(1, 5, 5), 3);
    BOOST_CHECK_EQUAL(get_sin_f(2, 5, 5), 5);
    BOOST_CHECK_EQUAL(get_sin_f(3, 5, 5), 4);
    BOOST_CHECK_EQUAL(get_sin_f(4, 5, 5), 2);

    BOOST_CHECK_EQUAL(get_sin_f(0, 4, 3), 0);
    BOOST_CHECK_EQUAL(get_sin_f(1, 4, 3), 2);
    BOOST_CHECK_EQUAL(get_sin_f(2, 4, 3), 3);
    BOOST_CHECK_EQUAL(get_sin_f(3, 4, 3), 1);
}

SCORUM_TEST_CASE(predict_input_check)
{
    // check methods for etalon schedule

    BOOST_CHECK_EQUAL(schedule_input_max_pos(), (uint64_t)8);

    BOOST_CHECK_EQUAL((share_type)schedule_input[0].bonus_percent, schedule_input_bonus_percent_by_pos(0));
    BOOST_CHECK_EQUAL((share_type)schedule_input[0].bonus_percent, schedule_input_bonus_percent_by_pos(1));

    BOOST_CHECK_EQUAL((share_type)schedule_input[1].bonus_percent, schedule_input_bonus_percent_by_pos(2));
    BOOST_CHECK_EQUAL((share_type)schedule_input[1].bonus_percent, schedule_input_bonus_percent_by_pos(3));

    BOOST_CHECK_EQUAL((share_type)schedule_input[2].bonus_percent, schedule_input_bonus_percent_by_pos(4));

    BOOST_CHECK_EQUAL((share_type)schedule_input[3].bonus_percent, schedule_input_bonus_percent_by_pos(5));
    BOOST_CHECK_EQUAL((share_type)schedule_input[3].bonus_percent, schedule_input_bonus_percent_by_pos(6));
    BOOST_CHECK_EQUAL((share_type)schedule_input[3].bonus_percent, schedule_input_bonus_percent_by_pos(7));

    BOOST_CHECK_EQUAL((share_type)0, schedule_input_bonus_percent_by_pos(8));

    asset maximum_bonus(5, REGISTRATION_BONUS_SYMBOL);

    BOOST_CHECK_EQUAL((share_type)schedule_input[0].bonus_percent * maximum_bonus.amount / 100,
                      schedule_input_predicted_allocate_by_pos(0, maximum_bonus).amount);
    BOOST_CHECK_EQUAL((share_type)schedule_input[1].bonus_percent * maximum_bonus.amount / 100,
                      schedule_input_predicted_allocate_by_pos(2, maximum_bonus).amount);
    BOOST_CHECK_EQUAL((share_type)schedule_input[2].bonus_percent * maximum_bonus.amount / 100,
                      schedule_input_predicted_allocate_by_pos(4, maximum_bonus).amount);
    BOOST_CHECK_EQUAL((share_type)schedule_input[3].bonus_percent * maximum_bonus.amount / 100,
                      schedule_input_predicted_allocate_by_pos(6, maximum_bonus).amount);

    uint64_t pos = 0;
    uint64_t max_pos = schedule_input_max_pos();
    asset predicted_limit(10, REGISTRATION_BONUS_SYMBOL);
    auto fn = [this, maximum_bonus](uint64_t p) -> asset {
        return this->schedule_input_predicted_allocate_by_pos(p, maximum_bonus);
    };

    BOOST_REQUIRE(schedule_input_pos_reach_limit(pos, max_pos, predicted_limit, maximum_bonus, fn));

    BOOST_REQUIRE_EQUAL(pos, 2); // pass only first stage of schedule

    BOOST_REQUIRE(schedule_input_pos_reach_limit(pos, max_pos, predicted_limit, maximum_bonus, fn));

    BOOST_REQUIRE_EQUAL(pos, 7); // pass through 2 stages of schedule (1 and 2)

    // limit is not reached (out of schedule)
    BOOST_REQUIRE(!schedule_input_pos_reach_limit(pos, max_pos, predicted_limit, maximum_bonus, fn));

    BOOST_REQUIRE_EQUAL(pos, max_pos); // out of schedule
}

SCORUM_TEST_CASE(allocate_through_all_schedule_stages_per_one_block_check)
{
    const registration_pool_object& pool = registration_pool_service.get_pool();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    // alice registers users per block from stage #1 to #schedule_input.size() (step is named as input_pos)
    auto it = schedule_input.begin();
    for (uint64_t input_pos = 0, stage = 0; it != schedule_input.end(); ++it, ++stage)
    {
        for (uint64_t register_attempts = 0; register_attempts < (uint64_t)(*it).users;
             ++register_attempts, ++input_pos)
        {
            generate_block();

            asset predicted_bonus = schedule_input_predicted_allocate_by_pos(input_pos, pool.maximum_bonus);

            asset allocated_bonus(0, REGISTRATION_BONUS_SYMBOL);
            db_plugin->debug_update(
                [&](database&) { allocated_bonus = registration_pool_service.allocate_cash("alice"); }, default_skip);

            BOOST_REQUIRE_EQUAL(allocated_bonus, predicted_bonus);
        }
    }
}

SCORUM_TEST_CASE(allocate_through_all_schedule_stages_per_wave_block_distribution_check)
{
    const registration_pool_object& pool = registration_pool_service.get_pool();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    uint64_t input_max_pos = schedule_input_max_pos();
    asset maximum_bonus = pool.maximum_bonus;

    // alice registers users per block from stage #1 to #schedule_input_size (step is named as input_pos)
    auto it = schedule_input.begin();
    for (uint64_t input_pos = 0, stage = 0; it != schedule_input.end(); ++it, ++stage)
    {
        for (uint64_t register_attempts = 0; register_attempts < (uint64_t)(*it).users;
             ++register_attempts, ++input_pos)
        {
            uint64_t ampl = get_sin_f(input_pos, input_max_pos, SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK + 1);
            // produce enough block amount to break out of sliding window in center of schedule
            for (uint64_t cblock = 0; cblock < ampl; ++cblock)
            {
                generate_block();
            }

            asset predicted_bonus = schedule_input_predicted_allocate_by_pos(input_pos, maximum_bonus);

            asset allocated_bonus(0, REGISTRATION_BONUS_SYMBOL);
            db_plugin->debug_update(
                [&](database&) { allocated_bonus = registration_pool_service.allocate_cash("alice"); }, default_skip);

            BOOST_CHECK_EQUAL(allocated_bonus, predicted_bonus);
        }
    }
}

SCORUM_TEST_CASE(allocate_limits_check)
{
    const registration_pool_object& pool = registration_pool_service.get_pool();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    asset predicted_limit = schedule_input_predicted_limit(1);

    auto fn = [&](uint64_t) -> asset {
        asset allocated_bonus(registration_pool_service.allocate_cash("alice").amount, REGISTRATION_BONUS_SYMBOL);
        BOOST_CHECK_GT(allocated_bonus.amount, share_type(0));
        return allocated_bonus;
    };

    uint64_t input_pos = 0;
    bool found_limit
        = schedule_input_pos_reach_limit(input_pos, schedule_input_max_pos(), predicted_limit, pool.maximum_bonus, fn);
    BOOST_REQUIRE(found_limit);

    // limit must be reached
    BOOST_REQUIRE_THROW(registration_pool_service.allocate_cash("alice"), fc::assert_exception);
}

SCORUM_TEST_CASE(allocate_limits_through_blocks_check)
{
    const registration_pool_object& pool = registration_pool_service.get_pool();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    // 1 = minimal (for calling without any block between allocate_cash)
    asset predicted_limit = schedule_input_predicted_limit(1);

    asset allocated_bonus(0, REGISTRATION_BONUS_SYMBOL);
    auto fn = [&](uint64_t) -> asset {
        db_plugin->debug_update([&](database&) { allocated_bonus = registration_pool_service.allocate_cash("alice"); },
                                default_skip);
        return allocated_bonus;
    };

    uint64_t input_pos = 0;
    uint64_t input_max_pos = schedule_input_max_pos();

    generate_block();

    bool found_limit
        = schedule_input_pos_reach_limit(input_pos, input_max_pos, predicted_limit, pool.maximum_bonus, fn);
    BOOST_REQUIRE(found_limit);

    for (int ci = 0; ci < SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK - (int)input_pos; ++ci)
    {
        generate_block(); // enlarge limit
    }

    for (int ci = 0; ci < SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK - (int)input_pos; ++ci)
    {
        fn(0); // get one bonus
    }

    // limit must be reached
    db_plugin->debug_update(
        [&](database&) { BOOST_REQUIRE_THROW(registration_pool_service.allocate_cash("alice"), fc::assert_exception); },
        default_skip);
}

SCORUM_TEST_CASE(allocate_limits_through_blocks_through_window_check)
{
    const registration_pool_object& pool = registration_pool_service.get_pool();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    asset predicted_limit = schedule_input_predicted_limit(1);

    asset allocated_bonus(0, REGISTRATION_BONUS_SYMBOL);
    auto fn = [&](uint64_t) -> asset {
        db_plugin->debug_update(
            [&](database&) {
                allocated_bonus = registration_pool_service.allocate_cash("alice");
                BOOST_CHECK_GT(allocated_bonus.amount, 0);
            },
            default_skip);
        return allocated_bonus;
    };

    uint64_t input_pos = 0;
    uint64_t input_max_pos = schedule_input_max_pos();

    generate_block();

    bool found_limit
        = schedule_input_pos_reach_limit(input_pos, input_max_pos, predicted_limit, pool.maximum_bonus, fn);
    BOOST_REQUIRE(found_limit);

    // move to next sliding window pos (reset limit)
    for (int ci = input_pos; ci < SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK + 1; ++ci)
    {
        generate_block();
    }

    predicted_limit = schedule_input_predicted_limit(SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK);

    // find next limit
    found_limit = schedule_input_pos_reach_limit(input_pos, input_max_pos, predicted_limit, pool.maximum_bonus, fn);
    BOOST_REQUIRE(found_limit);

    // limit must be reached
    db_plugin->debug_update(
        [&](database&) { BOOST_REQUIRE_THROW(registration_pool_service.allocate_cash("alice"), fc::assert_exception); },
        default_skip);
}

SCORUM_TEST_CASE(allocate_out_of_schedule_remain_check)
{
    const registration_pool_object& pool = registration_pool_service.get_pool();

    generate_block();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    uint64_t input_max_pos = schedule_input_max_pos();

    asset total_balance = pool.balance;
    asset total_allocated_bonus(0, REGISTRATION_BONUS_SYMBOL);

    // alice registers users per block from stage #1 to #schedule_input_size (step is named as input_pos)
    auto fn = [&]() -> asset {
        // to prevent reaching limit
        for (uint64_t cblock = 0; cblock < SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK; ++cblock)
        {
            generate_block();
        }
        asset allocated_bonus(0, REGISTRATION_BONUS_SYMBOL);
        db_plugin->debug_update([&](database&) { allocated_bonus = registration_pool_service.allocate_cash("alice"); },
                                default_skip);
        return allocated_bonus;
    };
    total_allocated_bonus = schedule_input_vest_all(fn);

    BOOST_CHECK_EQUAL(total_allocated_bonus, schedule_input_total_bonus(pool.maximum_bonus));
    BOOST_CHECK_EQUAL(pool.balance, total_balance - total_allocated_bonus);
    BOOST_CHECK_EQUAL(pool.already_allocated_count, input_max_pos);
    BOOST_CHECK_EQUAL(pool.balance, rest_of_supply);
}

SCORUM_TEST_CASE(autoclose_pool_with_valid_vesting_rest_check)
{
    const registration_pool_object& pool = registration_pool_service.get_pool();

    asset total_allocated_bonus(0, REGISTRATION_BONUS_SYMBOL);

    // alice registers users per block from stage #1 to #schedule_input_size (step is named as input_pos)
    auto fn = [&]() -> asset {
        // to prevent reaching limit
        for (uint64_t cblock = 0; cblock < SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK; ++cblock)
        {
            generate_block();
        }
        asset allocated_bonus(0, REGISTRATION_BONUS_SYMBOL);
        db_plugin->debug_update([&](database&) { allocated_bonus = registration_pool_service.allocate_cash("alice"); },
                                default_skip);
        return allocated_bonus;
    };
    total_allocated_bonus = schedule_input_vest_all(fn);

    BOOST_REQUIRE_EQUAL(total_allocated_bonus, schedule_input_total_bonus(pool.maximum_bonus));

    BOOST_REQUIRE_GT(rest_of_supply, asset(0, REGISTRATION_BONUS_SYMBOL));

    asset last_bonus = schedule_input_predicted_allocate_by_pos(schedule_input_max_pos() - 1, pool.maximum_bonus);

    BOOST_REQUIRE_GT(last_bonus, asset(0, REGISTRATION_BONUS_SYMBOL));

    int last_regs = (int)rest_of_supply.amount.value / (int)last_bonus.amount.value;

    BOOST_REQUIRE_GT(last_regs, 0);
    for (int ci = 0; ci < last_regs; ++ci)
    {
        // to prevent reaching limit
        for (uint64_t cblock = 0; cblock < SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK; ++cblock)
        {
            generate_block();
        }
        db_plugin->debug_update([&](database&) { registration_pool_service.allocate_cash("alice"); }, default_skip);
    }

    BOOST_REQUIRE_THROW(registration_pool_service.get_pool(), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
