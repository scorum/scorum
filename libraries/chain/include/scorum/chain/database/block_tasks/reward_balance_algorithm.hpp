#pragma once

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {

namespace database_ns {

using namespace scorum::protocol;

template <class TFundService> class reward_balance_algorithm
{
    TFundService& _service;

    const percent_type _adjust_percent;
    const uint32_t _guaranted_reward_supply_period_in_days;
    const uint32_t _reward_increase_threshold_in_days;

public:
    reward_balance_algorithm(TFundService& reward_service,
                             percent_type adjust_percent = SCORUM_ADJUST_REWARD_PERCENT,
                             uint32_t guaranted_reward_supply_period_in_days
                             = SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS,
                             uint32_t reward_increase_threshold_in_days = SCORUM_REWARD_INCREASE_THRESHOLD_IN_DAYS)
        : _service(reward_service)
        , _adjust_percent(adjust_percent)
        , _guaranted_reward_supply_period_in_days(guaranted_reward_supply_period_in_days)
        , _reward_increase_threshold_in_days(reward_increase_threshold_in_days)
    {
    }

    // return actual balance after increasing
    const asset& increase_ballance(const asset& delta)
    {
        _service.update([&](typename TFundService::object_type& pool) { pool.balance += delta; });

        return _service.get().balance;
    }

    const asset take_block_reward()
    {
        const auto current_per_day_reward = _service.get().current_per_block_reward * SCORUM_BLOCKS_PER_DAY;
        const auto symbol_type = current_per_day_reward.symbol();

        asset real_per_block_reward(0, symbol_type);

        _service.update([&](typename TFundService::object_type& pool) {
            asset delta = pool.current_per_block_reward * _adjust_percent / SCORUM_100_PERCENT;

            delta = std::max(asset(SCORUM_MIN_PER_BLOCK_REWARD, symbol_type), delta);

            if (pool.balance > current_per_day_reward * _reward_increase_threshold_in_days)
            {
                // recalculate
                pool.current_per_block_reward += delta;
            }
            else if (pool.balance < current_per_day_reward * _guaranted_reward_supply_period_in_days)
            {
                // recalculate
                pool.current_per_block_reward
                    = std::max(asset(SCORUM_MIN_PER_BLOCK_REWARD, symbol_type), pool.current_per_block_reward - delta);
            }
            else
            {
                // use current_perblock_reward
            }

            if (pool.balance >= pool.current_per_block_reward) // balance must not be negative
            {
                pool.balance -= pool.current_per_block_reward;
                real_per_block_reward = pool.current_per_block_reward;
            }
            else if (pool.balance.amount > 0)
            {
                real_per_block_reward = pool.balance;
                pool.balance.amount = 0;
            }
        });

        return real_per_block_reward;
    }
};
}
}
}
