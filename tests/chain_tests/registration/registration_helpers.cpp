#include "registration_helpers.hpp"

#include <boost/test/unit_test.hpp>

#include "registration_check_common.hpp"

#include "defines.hpp"

using namespace database_fixture;

namespace registration_helpers {

uint64_t fuzzer::get_sin_f(uint64_t pos, uint64_t max_pos, uint64_t max_result)
{
    BOOST_REQUIRE(max_pos > 0);

    // calculate upper part of the sin-curve

    const float fpi = 3.141592653589793f;
    float fpos = pos * fpi;
    fpos /= max_pos;

    float ret = std::sin(fpos);
    ret *= max_result;

    // roound to unsigned int type

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

BOOST_FIXTURE_TEST_SUITE(fuzzer_check, fuzzer)

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

BOOST_AUTO_TEST_SUITE_END()

void predictor::initialize(const asset& registration_supply,
                           const asset& registration_bonus,
                           const schedule_inputs_type& registration_schedule)
{
    _registration_supply = registration_supply;
    _registration_bonus = registration_bonus;
    _registration_schedule = registration_schedule;
}

uint64_t predictor::schedule_input_max_pos()
{
    uint64_t ret = 0;
    for (const auto& item : get_registration_schedule())
    {
        ret += item.users;
    }
    return ret;
}

asset predictor::schedule_input_total_bonus(const asset& maximum_bonus)
{
    return database_fixture::schedule_input_total_bonus(get_registration_schedule(), maximum_bonus);
}

int predictor::schedule_input_bonus_percent_by_pos(uint64_t pos)
{
    uint64_t rest = pos;
    auto it = get_registration_schedule().begin();
    for (uint64_t stage = 0; it != get_registration_schedule().end(); ++it, ++stage)
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

    if (it != get_registration_schedule().end())
    {
        return (*it).bonus_percent;
    }
    else
    {
        return 0;
    }
}

asset predictor::schedule_input_predicted_allocate_by_pos(uint64_t pos, asset maximum_bonus)
{
    share_type ret = (share_type)schedule_input_bonus_percent_by_pos(pos);
    ret *= maximum_bonus.amount;
    ret /= 100;
    return asset(ret, maximum_bonus.symbol());
}

asset predictor::schedule_input_predicted_limit(uint64_t pass_through_blocks)
{
    asset ret = get_registration_bonus();
    ret *= pass_through_blocks;
    ret *= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
    ret /= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
    return ret;
}

bool predictor::schedule_input_pos_reach_limit(uint64_t& pos,
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

        if (allocated + predicted_bonus > predicted_limit)
        {
            // limit is reached
            return true;
        }

        allocated += allocate_cash(pos++);
    }
    return false;
}

asset predictor::schedule_input_vest_all(std::function<asset()> allocate_cash)
{
    asset total_allocated_bonus(0, SCORUM_SYMBOL);
    auto it = get_registration_schedule().begin();
    for (uint64_t input_pos = 0, stage = 0; it != get_registration_schedule().end(); ++it, ++stage)
    {
        for (uint64_t register_attempts = 0; register_attempts < (uint64_t)(*it).users;
             ++register_attempts, ++input_pos)
        {
            total_allocated_bonus += allocate_cash();
        }
    }
    return total_allocated_bonus;
}

struct predictor_fixture : public database_fixture::registration_check_fixture, public predictor
{
    predictor_fixture()
    {
        database_fixture::schedule_inputs_type schedule_input = create_registration_genesis().registration_schedule;
        initialize(registration_supply(), registration_bonus(), schedule_input);
    }
};

BOOST_FIXTURE_TEST_SUITE(predictor_check, predictor_fixture)

SCORUM_TEST_CASE(schedule_input_bonus_percent_by_pos_check)
{
    size_t accounts = 0;
    for (size_t stages = 0; stages < get_registration_schedule().size(); ++stages)
    {
        for (size_t stage_accounts = 0; stage_accounts < get_registration_schedule()[stages].users;
             ++stage_accounts, ++accounts)
        {
            BOOST_CHECK_EQUAL((share_type)get_registration_schedule()[stages].bonus_percent,
                              schedule_input_bonus_percent_by_pos(accounts));
        }
    }
}

SCORUM_TEST_CASE(schedule_input_predicted_allocate_by_pos_check)
{
    asset maximum_bonus(5, SCORUM_SYMBOL);

    size_t pos = 0;
    for (size_t stages = 0; stages < get_registration_schedule().size(); ++stages)
    {
        BOOST_CHECK_EQUAL((share_type)get_registration_schedule()[stages].bonus_percent * maximum_bonus.amount / 100,
                          schedule_input_predicted_allocate_by_pos(pos, maximum_bonus).amount);
        pos += get_registration_schedule()[stages].users;
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

BOOST_AUTO_TEST_SUITE_END()
}
