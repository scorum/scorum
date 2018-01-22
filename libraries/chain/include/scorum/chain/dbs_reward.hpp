#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

#include <scorum/chain/pool/reward_pool.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class reward_pool_object;

class dbs_reward : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward(database& db);

public:
    const reward_pool_object& create_pool(const asset& initial_supply);

    const reward_pool_object& get_pool() const;

    // return actual balance after increasing
    const asset& increase_pool_ballance(const asset& delta);

    const asset take_block_reward();
};

} // namespace chain
} // namespace scorum
