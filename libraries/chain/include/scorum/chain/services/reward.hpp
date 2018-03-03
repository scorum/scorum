#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/reward_pool_object.hpp>

namespace scorum {
namespace chain {

struct reward_service_i : public base_service_i<reward_pool_object>
{
    virtual const reward_pool_object& create_pool(const asset& initial_supply) = 0;

    virtual const asset& increase_pool_ballance(const asset& delta) = 0;

    virtual const asset take_block_reward() = 0;
};

class dbs_reward : public dbs_service_base<reward_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward(database& db);

public:
    const reward_pool_object& create_pool(const asset& initial_supply) override;

    // return actual balance after increasing
    const asset& increase_pool_ballance(const asset& delta) override;

    const asset take_block_reward() override;
};

} // namespace chain
} // namespace scorum
