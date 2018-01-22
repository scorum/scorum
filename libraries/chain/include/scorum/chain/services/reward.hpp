#pragma once

#include <scorum/chain/services/base_service.hpp>

#include <scorum/chain/schema/reward_pool_object.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class reward_pool_object;

struct reward_service_i
{
    virtual const reward_pool_object& create_pool(const asset& initial_supply) = 0;

    virtual const reward_pool_object& get_pool() const = 0;

    virtual const asset& increase_pool_ballance(const asset& delta) = 0;

    virtual const asset take_block_reward() = 0;
};

class dbs_reward : public dbs_base, public reward_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward(database& db);

public:
    const reward_pool_object& create_pool(const asset& initial_supply) override;

    const reward_pool_object& get_pool() const override;

    // return actual balance after increasing
    const asset& increase_pool_ballance(const asset& delta) override;

    const asset take_block_reward() override;
};

} // namespace chain
} // namespace scorum
