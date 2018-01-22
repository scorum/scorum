#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

#include <scorum/chain/scorum_objects.hpp>

namespace scorum {
namespace chain {

class reward_fund_object;

struct reward_fund_service_i
{
    virtual const reward_fund_object& get_reward_fund() const = 0;
};

class dbs_reward_fund : public reward_fund_service_i, public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward_fund(database& db);

public:
    virtual const reward_fund_object& get_reward_fund() const override;
};

} // namespace scorum
} // namespace chain
