#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class hardfork_property_object;

struct hardfork_property_service_i
{
    virtual const hardfork_property_object& get() const = 0;

    using modifier_type = std::function<void(hardfork_property_object&)>;

    virtual const hardfork_property_object& create(const modifier_type& modifier) = 0;

    virtual void update(const modifier_type& modifier) = 0;

    virtual void remove() = 0;

    virtual bool is_exists() const = 0;
};

class dbs_hardfork_property : public dbs_base, public hardfork_property_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_hardfork_property(database& db);

public:
    virtual const hardfork_property_object& get() const override;

    virtual const hardfork_property_object& create(const modifier_type& modifier) override;

    virtual void update(const modifier_type& modifier) override;

    virtual void remove() override;

    virtual bool is_exists() const override;
};
} // namespace chain
} // namespace scorum
