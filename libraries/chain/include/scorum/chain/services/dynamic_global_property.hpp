#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class dynamic_global_property_object;

struct dynamic_global_property_service_i
{
    virtual time_point_sec get_genesis_time() const = 0;

    virtual const dynamic_global_property_object& get() const = 0;

    virtual bool is_exists() const = 0;

    using modifier_type = std::function<void(dynamic_global_property_object&)>;

    virtual const dynamic_global_property_object& create(const modifier_type& modifier) = 0;

    virtual void update(const modifier_type& modifier) = 0;

    virtual fc::time_point_sec head_block_time() const = 0;
};

class dbs_dynamic_global_property : public dbs_base, public dynamic_global_property_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_dynamic_global_property(database& db);

public:
    virtual time_point_sec get_genesis_time() const override;

    virtual const dynamic_global_property_object& get() const override;

    virtual bool is_exists() const override;

    virtual const dynamic_global_property_object& create(const modifier_type& modifier) override;

    virtual void update(const modifier_type& modifier) override;

    virtual fc::time_point_sec head_block_time() const override;
};

} // namespace chain
} // namespace scorum
