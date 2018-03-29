#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class reward_balancer_object;

struct reward_service_i
{
    using modifier_type = std::function<void(reward_balancer_object&)>;
    virtual void update(const modifier_type& modifier) = 0;

    virtual const reward_balancer_object& create_balancer(const asset& initial_supply) = 0;

    virtual bool is_exists() const = 0;

    virtual const reward_balancer_object& get() const = 0;

    virtual const asset& increase_ballance(const asset& delta) = 0;

    virtual const asset take_block_reward() = 0;
};

class dbs_reward : public dbs_base, public reward_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward(database& db);

public:
    virtual void update(const modifier_type& modifier) override;

    const reward_balancer_object& create_balancer(const asset& initial_supply) override;

    bool is_exists() const override;

    const reward_balancer_object& get() const override;

    // return actual balance after increasing
    const asset& increase_ballance(const asset& delta) override;

    const asset take_block_reward() override;
};

} // namespace chain
} // namespace scorum
