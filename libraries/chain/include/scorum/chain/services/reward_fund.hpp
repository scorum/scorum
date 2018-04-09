#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class reward_fund_object;

struct reward_fund_service_i
{
    virtual bool is_exists() const = 0;

    virtual const reward_fund_object& get() const = 0;

    using modifier_type = std::function<void(reward_fund_object&)>;

    virtual const reward_fund_object& create(const modifier_type& modifier) = 0;

    virtual void update(const modifier_type& modifier) = 0;
};

class dbs_reward_fund : public dbs_base, public reward_fund_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward_fund(database& db);

public:
    bool is_exists() const override;

    const reward_fund_object& get() const override;

    const reward_fund_object& create(const modifier_type& modifier) override;

    void update(const modifier_type& modifier) override;
};

} // namespace scorum
} // namespace chain
