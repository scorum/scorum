#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class dev_committee_object;

struct dev_pool_service_i
{
    virtual const dev_committee_object& get() const = 0;

    virtual bool is_exists() const = 0;

    using modifier_type = std::function<void(dev_committee_object&)>;

    virtual const dev_committee_object& create(const modifier_type&) = 0;

    virtual void update(const modifier_type&) = 0;

    virtual asset get_sp_balance() const = 0;
    virtual asset get_scr_balace() const = 0;

    virtual void decrease_scr_balance(const asset& amount) = 0;
};

class dbs_dev_pool : public dbs_base, public dev_pool_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_dev_pool(database& db);

public:
    virtual const dev_committee_object& get() const override;

    virtual bool is_exists() const override;

    virtual const dev_committee_object& create(const modifier_type&) override;

    virtual void update(const modifier_type&) override;

    asset get_sp_balance() const override;
    asset get_scr_balace() const override;

    void decrease_scr_balance(const asset& amount) override;
};
} // namespace chain
} // namespace scorum
