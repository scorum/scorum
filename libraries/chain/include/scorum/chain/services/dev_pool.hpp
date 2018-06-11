#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

struct dev_pool_service_i : public base_service_i<dev_committee_object>
{
    virtual asset get_scr_balace() const = 0;

    virtual void decrease_scr_balance(const asset& amount) = 0;
};

class dbs_dev_pool : public dbs_service_base<dev_pool_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_dev_pool(database& db);

public:
    asset get_scr_balace() const override;

    void decrease_scr_balance(const asset& amount) override;
};
} // namespace chain
} // namespace scorum
