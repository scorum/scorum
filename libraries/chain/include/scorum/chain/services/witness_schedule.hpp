#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class witness_schedule_object;

struct witness_schedule_service_i
{
    virtual const witness_schedule_object& get() const = 0;

    using modifier_type = std::function<void(witness_schedule_object&)>;

    virtual const witness_schedule_object& create(const modifier_type& modifier) = 0;

    virtual void update(const modifier_type& modifier) = 0;

    virtual void remove() = 0;

    virtual bool is_exists() const = 0;
};

class dbs_witness_schedule : public dbs_base, public witness_schedule_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_witness_schedule(database& db);

public:
    virtual const witness_schedule_object& get() const override;

    virtual const witness_schedule_object& create(const modifier_type& modifier) override;

    virtual void update(const modifier_type& modifier) override;

    virtual void remove() override;

    virtual bool is_exists() const override;
};
} // namespace chain
} // namespace scorum
