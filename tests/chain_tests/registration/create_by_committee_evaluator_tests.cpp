#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <math.h>

#include "registration_check_common.hpp"
#include "registration_helpers.hpp"
#include "actor.hpp"

#include <scorum/chain/evaluators/registration_pool_evaluator.hpp>

#include <sstream>

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace registration_fixtures;

class create_by_committee_evaluator_check_fixture : public registration_check_fixture
{
public:
    create_by_committee_evaluator_check_fixture()
    {
        genesis_state = create_registration_genesis(schedule_input);
        create_registration_objects(genesis_state);
        predictor.initialize(registration_supply(), registration_bonus(), schedule_input);
    }

    schedule_inputs_type schedule_input;
    registration_helpers::predictor predictor;
    registration_helpers::fuzzer fuzzer;

    const account_object& create_new_by_committee()
    {
        static int next_id = 1;
        std::stringstream store;
        store << "user" << next_id++;
        Actor new_account(store.str());

        account_create_by_committee_operation op;
        op.creator = committee_member().name;
        op.new_account_name = new_account.name;
        op.owner = authority(1, new_account.public_key, 1);
        op.active = authority(1, new_account.public_key, 1);
        op.posting = authority(1, new_account.public_key, 1);
        op.memo_key = new_account.public_key;
        op.json_metadata = "";

        registration_pool_evaluator ev(db);
        ev.do_apply(op);

        return account_service.get_account(new_account.name);
    }
};

BOOST_FIXTURE_TEST_SUITE(create_by_committee_evaluator_check, create_by_committee_evaluator_check_fixture)

SCORUM_TEST_CASE(allocate_through_all_schedule_stages_per_one_block_check)
{
    const registration_pool_object& pool = registration_pool_service.get();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    // alice registers users per block from stage #1 to #schedule_input.size() (step is named as input_pos)
    auto it = schedule_input.begin();
    for (uint64_t input_pos = 0, stage = 0; it != schedule_input.end(); ++it, ++stage)
    {
        for (uint64_t register_attempts = 0; register_attempts < (uint64_t)(*it).users;
             ++register_attempts, ++input_pos)
        {
            generate_block();

            asset predicted_bonus = predictor.schedule_input_predicted_allocate_by_pos(input_pos, pool.maximum_bonus);

            asset allocated_bonus(0, SCORUM_SYMBOL);
            db_plugin->debug_update(
                [&](database&) {
                    const account_object& account = create_new_by_committee();
                    allocated_bonus = asset(account.scorumpower.amount, SCORUM_SYMBOL);
                },
                default_skip);

            BOOST_REQUIRE_EQUAL(allocated_bonus, predicted_bonus);
        }
    }
}

SCORUM_TEST_CASE(allocate_through_all_schedule_stages_per_wave_block_distribution_check)
{
    const registration_pool_object& pool = registration_pool_service.get();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    uint64_t input_max_pos = predictor.schedule_input_max_pos();
    asset maximum_bonus = pool.maximum_bonus;

    // alice registers users per block from stage #1 to #schedule_input_size (step is named as input_pos)
    auto it = schedule_input.begin();
    for (uint64_t input_pos = 0, stage = 0; it != schedule_input.end(); ++it, ++stage)
    {
        for (uint64_t register_attempts = 0; register_attempts < (uint64_t)(*it).users;
             ++register_attempts, ++input_pos)
        {
            uint64_t ampl
                = fuzzer.get_sin_f(input_pos, input_max_pos, SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK + 1);
            // produce enough block amount to break out of sliding window in center of schedule
            ampl++; // make at least one generate_block
            for (uint64_t cblock = 0; cblock < ampl; ++cblock)
            {
                generate_block();
            }

            asset predicted_bonus = predictor.schedule_input_predicted_allocate_by_pos(input_pos, maximum_bonus);

            asset allocated_bonus(0, SCORUM_SYMBOL);
            db_plugin->debug_update(
                [&](database&) {
                    const account_object& account = create_new_by_committee();
                    allocated_bonus = asset(account.scorumpower.amount, SCORUM_SYMBOL);
                },
                default_skip);

            BOOST_CHECK_EQUAL(allocated_bonus, predicted_bonus);
        }
    }
}

SCORUM_TEST_CASE(allocate_limits_check)
{
    const registration_pool_object& pool = registration_pool_service.get();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    asset predicted_limit = predictor.schedule_input_predicted_limit(1);

    auto fn = [&](uint64_t) -> asset {
        const account_object& account = create_new_by_committee();
        asset allocated_bonus(account.scorumpower.amount, SCORUM_SYMBOL);
        return allocated_bonus;
    };

    uint64_t input_pos = 0;
    bool found_limit = predictor.schedule_input_pos_reach_limit(input_pos, predictor.schedule_input_max_pos(),
                                                                predicted_limit, pool.maximum_bonus, fn);
    BOOST_REQUIRE(found_limit);

    // limit must be reached
    BOOST_REQUIRE_THROW(fn(0), fc::assert_exception);
}

SCORUM_TEST_CASE(allocate_limits_through_blocks_through_window_check)
{
    const registration_pool_object& pool = registration_pool_service.get();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    asset predicted_limit = predictor.schedule_input_predicted_limit(1);

    auto fn = [&](uint64_t) -> asset {
        const account_object& account = create_new_by_committee();
        asset allocated_bonus(account.scorumpower.amount, SCORUM_SYMBOL);
        return allocated_bonus;
    };

    asset allocated_bonus(0, SCORUM_SYMBOL);
    auto fn_db = [&](uint64_t) -> asset {
        db_plugin->debug_update(
            [&](database&) {
                // std::cout << allocated_bonus << std::endl;
                allocated_bonus = fn(0);
            },
            default_skip);
        return allocated_bonus;
    };

    uint64_t input_pos = 0;
    uint64_t input_max_pos = predictor.schedule_input_max_pos();

    generate_block();

    bool found_limit = predictor.schedule_input_pos_reach_limit(input_pos, input_max_pos, predicted_limit,
                                                                pool.maximum_bonus, fn_db);
    BOOST_REQUIRE(found_limit);

    BOOST_REQUIRE_THROW(fn(0), fc::assert_exception);

    // move to next sliding window pos (reset limit)
    for (int ci = 0; ci < SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK; ++ci)
    {
        generate_block();
    }

    predicted_limit = predictor.schedule_input_predicted_limit(1);

    // find next limit
    found_limit = predictor.schedule_input_pos_reach_limit(input_pos, input_max_pos, predicted_limit,
                                                           pool.maximum_bonus, fn_db);
    BOOST_REQUIRE(found_limit);

    // limit must be reached
    BOOST_REQUIRE_THROW(fn(0), fc::assert_exception);
}

SCORUM_TEST_CASE(allocate_out_of_schedule_remain_check)
{
    const registration_pool_object& pool = registration_pool_service.get();

    generate_block();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    uint64_t input_max_pos = predictor.schedule_input_max_pos();

    asset total_balance = pool.balance;
    asset total_allocated_bonus(0, SCORUM_SYMBOL);

    // alice registers users per block from stage #1 to #schedule_input_size (step is named as input_pos)
    auto fn = [&]() -> asset {

        // to prevent reaching limit
        generate_blocks(SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK);

        asset allocated_bonus(0, SCORUM_SYMBOL);
        db_plugin->debug_update(
            [&](database&) {
                const account_object& account = create_new_by_committee();
                allocated_bonus = asset(account.scorumpower.amount, SCORUM_SYMBOL);
            },
            default_skip);
        return allocated_bonus;
    };
    total_allocated_bonus = predictor.schedule_input_vest_all(fn);

    BOOST_CHECK_EQUAL(total_allocated_bonus, predictor.schedule_input_total_bonus(pool.maximum_bonus));
    BOOST_CHECK_EQUAL(pool.balance, total_balance - total_allocated_bonus);
    BOOST_CHECK_EQUAL(pool.already_allocated_count, input_max_pos);
    BOOST_CHECK_EQUAL(pool.balance, rest_of_supply());
}

SCORUM_TEST_CASE(autoclose_pool_with_valid_vesting_rest_check)
{
    const registration_pool_object& pool = registration_pool_service.get();

    asset total_allocated_bonus(0, SCORUM_SYMBOL);

    // alice registers users per block from stage #1 to #schedule_input_size (step is named as input_pos)
    auto fn = [&]() -> asset {

        // to prevent reaching limit
        generate_blocks(SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK);

        asset allocated_bonus(0, SCORUM_SYMBOL);
        db_plugin->debug_update(
            [&](database&) {
                const account_object& account = create_new_by_committee();
                allocated_bonus = asset(account.scorumpower.amount, SCORUM_SYMBOL);
            },
            default_skip);
        return allocated_bonus;
    };
    total_allocated_bonus = predictor.schedule_input_vest_all(fn);

    BOOST_REQUIRE_EQUAL(total_allocated_bonus, predictor.schedule_input_total_bonus(pool.maximum_bonus));

    BOOST_REQUIRE_GT(rest_of_supply(), asset(0, SCORUM_SYMBOL));

    asset last_bonus = predictor.schedule_input_predicted_allocate_by_pos(predictor.schedule_input_max_pos() - 1,
                                                                          pool.maximum_bonus);

    BOOST_REQUIRE_GT(last_bonus, asset(0, SCORUM_SYMBOL));

    int last_regs = (int)rest_of_supply().amount.value / (int)last_bonus.amount.value;

    BOOST_REQUIRE_GT(last_regs, 0);
    for (int ci = 0; ci < last_regs; ++ci)
    {
        // to prevent reaching limit
        generate_blocks(SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK);

        db_plugin->debug_update([&](database&) { create_new_by_committee(); }, default_skip);
    }

    BOOST_CHECK_EQUAL(registration_pool_service.get().balance, ASSET_NULL_SCR);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
