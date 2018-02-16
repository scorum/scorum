#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class dev_committee;

struct dev_pool_service_i
{
    virtual const dev_committee& get() const = 0;

    virtual bool is_exists() const = 0;

    virtual const dev_committee& create(const asset& balance) = 0;

    virtual void increase_balance(const asset& amount) = 0;
    virtual void decrease_balance(const asset& amount) = 0;
};

class dbs_dev_pool : public dbs_base, public dev_pool_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_dev_pool(database& db);

public:
    const dev_committee& get() const override;

    bool is_exists() const override;

    const dev_committee& create(const asset& balance) override;

    void increase_balance(const asset& amount) override;
    void decrease_balance(const asset& amount) override;
};
} // namespace chain
} // namespace scorum
