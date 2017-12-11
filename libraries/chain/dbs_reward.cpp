#include <scorum/chain/dbs_reward.hpp>
#include <scorum/chain/database.hpp>
#include <scorum/chain/pool/reward_pool.hpp>

namespace scorum {
namespace chain {

dbs_reward::dbs_reward(database& db)
    : _base_type(db)
{
}

const reward_pool_object& dbs_reward::create_pool(const asset& initial_supply)
{
    // clang-format off
    asset initial_per_block_reward(initial_supply.amount / (SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS * SCORUM_BLOCKS_PER_DAY), initial_supply.symbol);

    FC_ASSERT(db_impl().find<reward_pool_object>() == nullptr, "recreation of reward_pool_object is not allowed");
    FC_ASSERT(initial_supply > asset(0, SCORUM_SYMBOL), "initial supply for reward_pool must not be null");
    FC_ASSERT(initial_per_block_reward > asset(0, SCORUM_SYMBOL),
              "initial supply for reward_pool is not sufficient to make per_block_reward > 0. It should be at least ${1}, but current value is ${2}",
              ("1", asset(SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS * SCORUM_BLOCKS_PER_DAY)) ("2", initial_supply));

    return db_impl().create<reward_pool_object>([&](reward_pool_object& rp) {
        rp.balance = initial_supply;
        rp.current_per_block_reward = initial_per_block_reward;
    });
    // clang-format on
}

const reward_pool_object& dbs_reward::get_pool() const
{
    return db_impl().get<reward_pool_object>();
}

const asset& dbs_reward::increase_pool_ballance(const asset& delta)
{
    const auto& pool = get_pool();

    db_impl().modify(pool, [&](reward_pool_object& pool) {
        switch (delta.symbol)
        {
        case SCORUM_SYMBOL:
            FC_ASSERT(delta >= asset(0));
            pool.balance += delta;
            break;
        default:
            FC_ASSERT(false, "invalid symbol");
        }
    });

    return pool.balance;
}

// clang-format off
const asset& dbs_reward::take_block_reward()
{
    const auto& pool = get_pool();

    FC_ASSERT(pool.current_per_block_reward > asset(0));

    db_impl().modify(pool, [&](reward_pool_object& pool) 
    { 
        if (pool.balance > asset(pool.current_per_block_reward.amount * SCORUM_BLOCKS_PER_DAY * SCORUM_REWARD_INCREASE_THRESHOLD_IN_DAYS))
        {
            // recalculate
            pool.current_per_block_reward += asset((pool.current_per_block_reward.amount * SCORUM_ADJUST_REWARD_PERCENT) / 100);
        }
        else if (pool.balance < asset(pool.current_per_block_reward.amount * SCORUM_BLOCKS_PER_DAY * SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS))
        {
            // recalculate
            pool.current_per_block_reward -= asset((pool.current_per_block_reward.amount * SCORUM_ADJUST_REWARD_PERCENT) / 100);
        }
        else
        {
            // use current_perblock_reward
        }
        pool.balance -= pool.current_per_block_reward; 
    });

    return pool.current_per_block_reward;
}
// clang-format on

} // namespace chain
} // namespace scorum
