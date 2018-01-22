#pragma once

#include <scorum/chain/services/base_service.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

struct dynamic_global_property_service_i
{
    virtual const dynamic_global_property_object& get_dynamic_global_properties() const = 0;
    virtual fc::time_point_sec head_block_time() = 0;
};

class dbs_dynamic_global_property : public dbs_base, public dynamic_global_property_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_dynamic_global_property(database& db);

public:
    virtual const dynamic_global_property_object& get_dynamic_global_properties() const override;

    virtual fc::time_point_sec head_block_time() override;

    time_point_sec head_block_time() const;

    void set_invite_quorum(uint64_t quorum);
    void set_dropout_quorum(uint64_t quorum);
    void set_quorum(uint64_t quorum);
};

} // namespace chain
} // namespace scorum
