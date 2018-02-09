#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class reward_pool_object;
class reward_fund_object;

struct reward_service_i
{
    using modifier_type = std::function<void(reward_fund_object&)>;

    virtual const reward_fund_object& create_fund(const modifier_type& modifier) = 0;

    virtual bool is_fund_exists() const = 0;

    virtual const reward_pool_object& create_pool(const asset& initial_supply) = 0;

    virtual bool is_pool_exists() const = 0;

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
    const reward_fund_object& create_fund(const modifier_type& modifier) override;

    bool is_fund_exists() const override;

    const reward_pool_object& create_pool(const asset& initial_supply) override;

    bool is_pool_exists() const override;

    const reward_pool_object& get_pool() const override;

    // return actual balance after increasing
    const asset& increase_pool_ballance(const asset& delta) override;

    const asset take_block_reward() override;
};

} // namespace chain
} // namespace scorum
