#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class reward_fund_object;

struct reward_fund_service_i
{
    virtual const reward_fund_object& get_reward_fund() const = 0;
};

class dbs_reward_fund : public dbs_base, public reward_fund_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward_fund(database& db);

public:
    virtual const reward_fund_object& get_reward_fund() const override;
};

} // namespace scorum
} // namespace chain
