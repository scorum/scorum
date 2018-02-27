#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <math.h>

#include "registration_check_common.hpp"

#include <scorum/chain/evaluators/registration_pool_evaluator.hpp>

#include <sstream>

using namespace scorum::chain;
using namespace scorum::protocol;

class pool_evaluator_check_fixture : public registration_check_fixture
{
public:
    pool_evaluator_check_fixture()
    {
        genesis_state = create_registration_genesis(schedule_input);
        create_registration_objects(genesis_state);
    }

    scorum::chain::schedule_inputs_type schedule_input;

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

        if (ret < 0.f)
        {
            ret = 0.f;
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
        return scorum::chain::schedule_input_total_bonus(schedule_input, maximum_bonus);
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
        return asset(ret, maximum_bonus.symbol());
    }

    asset schedule_input_predicted_limit(uint64_t pass_through_blocks)
    {
        asset ret = registration_bonus();
        ret *= pass_through_blocks;
        ret *= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
        ret /= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
        return ret;
    }

    bool schedule_input_pos_reach_limit(uint64_t& pos,
                                        uint64_t max_pos,
                                        asset predicted_limit,
                                        asset maximum_bonus,
                                        std::function<asset(uint64_t pos)> allocate_cash)
    {
        asset allocated(0, SCORUM_SYMBOL);
        while (pos < max_pos)
        {
            asset predicted_bonus = schedule_input_predicted_allocate_by_pos(pos, maximum_bonus);

            if (predicted_bonus.amount == share_type(0))
            {
                break;
            }

            ++pos;

            if (allocated + predicted_bonus > predicted_limit)
            {
                // limit is reached
                return true;
            }

            allocated += allocate_cash(pos);
        }
        return false;
    }

    asset schedule_input_vest_all(std::function<asset()> allocate_cash)
    {
        asset total_allocated_bonus(0, SCORUM_SYMBOL);
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

    const account_object& create_new_by_committee()
    {
        static int next_id = 1;
        std::stringstream store;
        store << "user" << next_id++;

        std::string new_account_name(store.str());
        fc::ecc::private_key new_private_key = generate_private_key(new_account_name);
        fc::ecc::public_key new_public_key = new_private_key.get_public_key();

        account_create_by_committee_operation op;
        op.creator = committee_member().name;
        op.new_account_name = new_account_name;
        op.owner = authority(1, new_public_key, 1);
        op.active = authority(1, new_public_key, 1);
        op.posting = authority(1, new_public_key, 1);
        op.memo_key = new_public_key;
        op.json_metadata = "";

        registration_pool_evaluator ev(db);
        ev.do_apply(op);

        return account_service.get_account(new_account_name);
    }
};

BOOST_FIXTURE_TEST_SUITE(pool_evaluator_check, pool_evaluator_check_fixture)

SCORUM_TEST_CASE(sin_f_check)
{
    BOOST_CHECK_EQUAL(get_sin_f(0, 10, 5), (uint64_t)0);
    BOOST_CHECK_EQUAL(get_sin_f(1, 10, 5), (uint64_t)2);
    BOOST_CHECK_EQUAL(get_sin_f(2, 10, 5), (uint64_t)3);
    BOOST_CHECK_EQUAL(get_sin_f(3, 10, 5), (uint64_t)4);
    BOOST_CHECK_EQUAL(get_sin_f(4, 10, 5), (uint64_t)5);
    BOOST_CHECK_EQUAL(get_sin_f(5, 10, 5), (uint64_t)5);
    BOOST_CHECK_EQUAL(get_sin_f(6, 10, 5), (uint64_t)4);
    BOOST_CHECK_EQUAL(get_sin_f(7, 10, 5), (uint64_t)3);
    BOOST_CHECK_EQUAL(get_sin_f(8, 10, 5), (uint64_t)2);
    BOOST_CHECK_EQUAL(get_sin_f(9, 10, 5), (uint64_t)1);

    BOOST_CHECK_EQUAL(get_sin_f(0, 5, 5), (uint64_t)0);
    BOOST_CHECK_EQUAL(get_sin_f(1, 5, 5), (uint64_t)3);
    BOOST_CHECK_EQUAL(get_sin_f(2, 5, 5), (uint64_t)5);
    BOOST_CHECK_EQUAL(get_sin_f(3, 5, 5), (uint64_t)4);
    BOOST_CHECK_EQUAL(get_sin_f(4, 5, 5), (uint64_t)2);

    BOOST_CHECK_EQUAL(get_sin_f(0, 4, 3), (uint64_t)0);
    BOOST_CHECK_EQUAL(get_sin_f(1, 4, 3), (uint64_t)2);
    BOOST_CHECK_EQUAL(get_sin_f(2, 4, 3), (uint64_t)3);
    BOOST_CHECK_EQUAL(get_sin_f(3, 4, 3), (uint64_t)1);
}

SCORUM_TEST_CASE(schedule_input_bonus_percent_by_pos_check)
{
    size_t accounts = 0;
    for (size_t stages = 0; stages < schedule_input.size(); ++stages)
    {
        for (size_t stage_accounts = 0; stage_accounts < schedule_input[stages].users; ++stage_accounts, ++accounts)
        {
            BOOST_CHECK_EQUAL((share_type)schedule_input[stages].bonus_percent,
                              schedule_input_bonus_percent_by_pos(accounts));
        }
    }
}

SCORUM_TEST_CASE(schedule_input_predicted_allocate_by_pos_check)
{
    asset maximum_bonus(5, SCORUM_SYMBOL);

    size_t pos = 0;
    for (size_t stages = 0; stages < schedule_input.size(); ++stages)
    {
        BOOST_CHECK_EQUAL((share_type)schedule_input[stages].bonus_percent * maximum_bonus.amount / 100,
                          schedule_input_predicted_allocate_by_pos(pos, maximum_bonus).amount);
        pos += schedule_input[stages].users;
    }
}

SCORUM_TEST_CASE(schedule_input_pos_reach_limit_check)
{
    asset maximum_bonus(5, SCORUM_SYMBOL);

    uint64_t pos = 0;
    uint64_t max_pos = schedule_input_max_pos();
    asset predicted_limit = maximum_bonus * 2;
    auto fn = [this, maximum_bonus](uint64_t p) -> asset {
        return this->schedule_input_predicted_allocate_by_pos(p, maximum_bonus);
    };

    BOOST_REQUIRE(schedule_input_pos_reach_limit(pos, max_pos, predicted_limit, maximum_bonus, fn));

    BOOST_REQUIRE_LT(pos, max_pos);
}

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

            asset predicted_bonus = schedule_input_predicted_allocate_by_pos(input_pos, pool.maximum_bonus);

            asset allocated_bonus(0, SCORUM_SYMBOL);
            db_plugin->debug_update(
                [&](database&) {
                    const account_object& account = create_new_by_committee();
                    allocated_bonus = asset(account.vesting_shares.amount, SCORUM_SYMBOL);
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
            ampl++; // make at least one generate_block
            for (uint64_t cblock = 0; cblock < ampl; ++cblock)
            {
                generate_block();
            }

            asset predicted_bonus = schedule_input_predicted_allocate_by_pos(input_pos, maximum_bonus);

            asset allocated_bonus(0, SCORUM_SYMBOL);
            db_plugin->debug_update(
                [&](database&) {
                    const account_object& account = create_new_by_committee();
                    allocated_bonus = asset(account.vesting_shares.amount, SCORUM_SYMBOL);
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

    asset predicted_limit = schedule_input_predicted_limit(1);

    auto fn = [&](uint64_t) -> asset {
        const account_object& account = create_new_by_committee();
        asset allocated_bonus(account.vesting_shares.amount, SCORUM_SYMBOL);
        BOOST_CHECK_GT(allocated_bonus.amount, share_type(0));
        return allocated_bonus;
    };

    uint64_t input_pos = 0;
    bool found_limit
        = schedule_input_pos_reach_limit(input_pos, schedule_input_max_pos(), predicted_limit, pool.maximum_bonus, fn);
    BOOST_REQUIRE(found_limit);

    // limit must be reached
    BOOST_REQUIRE_THROW(fn(0), fc::assert_exception);
}

SCORUM_TEST_CASE(allocate_limits_through_blocks_through_window_check)
{
    const registration_pool_object& pool = registration_pool_service.get();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    asset predicted_limit = schedule_input_predicted_limit(1);

    asset allocated_bonus(0, SCORUM_SYMBOL);
    auto fn = [&](uint64_t) -> asset {
        db_plugin->debug_update(
            [&](database&) {
                const account_object& account = create_new_by_committee();
                allocated_bonus = asset(account.vesting_shares.amount, SCORUM_SYMBOL);
                BOOST_CHECK_GT(allocated_bonus.amount, share_type(0));
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

    BOOST_REQUIRE_THROW(fn(0), fc::assert_exception);

    // move to next sliding window pos (reset limit)
    for (int ci = 0; ci < SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK; ++ci)
    {
        generate_block();
    }

    predicted_limit = schedule_input_predicted_limit(1);

    // find next limit
    found_limit = schedule_input_pos_reach_limit(input_pos, input_max_pos, predicted_limit, pool.maximum_bonus, fn);
    BOOST_REQUIRE(found_limit);

    // limit must be reached
    BOOST_REQUIRE_THROW(fn(0), fc::assert_exception);
}

/*

SCORUM_TEST_CASE(allocate_out_of_schedule_remain_check)
{
    const registration_pool_object& pool = registration_pool_service.get();

    generate_block();

    BOOST_REQUIRE_EQUAL(pool.schedule_items.size(), schedule_input.size());

    uint64_t input_max_pos = schedule_input_max_pos();

    asset total_balance = pool.balance;
    asset total_allocated_bonus(0, SCORUM_SYMBOL);

    // alice registers users per block from stage #1 to #schedule_input_size (step is named as input_pos)
    auto fn = [&]() -> asset {

        // to prevent reaching limit
        generate_blocks(SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK);

        asset allocated_bonus(0, SCORUM_SYMBOL);
        db_plugin->debug_update(
            [&](database&) {
                allocated_bonus = registration_pool_service.allocate_cash("alice");
                // to save invariants we must give allocated bonus to someone
                account_service.create_vesting(bonus_beneficiary(), allocated_bonus);
            },
            default_skip);
        return allocated_bonus;
    };
    total_allocated_bonus = schedule_input_vest_all(fn);

    BOOST_CHECK_EQUAL(total_allocated_bonus, schedule_input_total_bonus(pool.maximum_bonus));
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
                allocated_bonus = registration_pool_service.allocate_cash("alice");
                // to save invariants we must give allocated bonus to someone
                account_service.create_vesting(bonus_beneficiary(), allocated_bonus);
            },
            default_skip);
        return allocated_bonus;
    };
    total_allocated_bonus = schedule_input_vest_all(fn);

    BOOST_REQUIRE_EQUAL(total_allocated_bonus, schedule_input_total_bonus(pool.maximum_bonus));

    BOOST_REQUIRE_GT(rest_of_supply(), asset(0, SCORUM_SYMBOL));

    asset last_bonus = schedule_input_predicted_allocate_by_pos(schedule_input_max_pos() - 1, pool.maximum_bonus);

    BOOST_REQUIRE_GT(last_bonus, asset(0, SCORUM_SYMBOL));

    int last_regs = (int)rest_of_supply().amount.value / (int)last_bonus.amount.value;

    BOOST_REQUIRE_GT(last_regs, 0);
    for (int ci = 0; ci < last_regs; ++ci)
    {
        // to prevent reaching limit
        generate_blocks(SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK);

        db_plugin->debug_update(
            [&](database&) {
                auto allocated_bonus = registration_pool_service.allocate_cash("alice");
                // to save invariants we must give allocated bonus to someone
                account_service.create_vesting(bonus_beneficiary(), allocated_bonus);
            },
            default_skip);
    }

    BOOST_REQUIRE_THROW(registration_pool_service.get(), fc::exception);
}
*/

BOOST_AUTO_TEST_SUITE_END()

#endif
