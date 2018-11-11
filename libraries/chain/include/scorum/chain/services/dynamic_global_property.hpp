#pragma once
#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

struct dynamic_global_property_service_i : public base_service_i<dynamic_global_property_object>
{
    virtual fc::time_point_sec head_block_time() const = 0;
    virtual uint32_t head_block_num() const = 0;
};

class dbs_dynamic_global_property : public dbs_service_base<dynamic_global_property_service_i>
{
    friend class dbservice_dbs_factory;

public:
    explicit dbs_dynamic_global_property(dba::db_index& db);

    virtual fc::time_point_sec head_block_time() const override;
    virtual uint32_t head_block_num() const override;
};

} // namespace chain
} // namespace scorum
