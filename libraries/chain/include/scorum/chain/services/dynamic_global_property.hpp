#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class dynamic_global_property_object;

struct dynamic_global_property_service_i
{
    virtual const dynamic_global_property_object& get() const = 0;
    virtual fc::time_point_sec head_block_time() const = 0;
};

class dbs_dynamic_global_property : public dbs_base, public dynamic_global_property_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_dynamic_global_property(database& db);

public:
    virtual const dynamic_global_property_object& get() const override;

    virtual fc::time_point_sec head_block_time() const override;

    void set_invite_quorum(uint64_t quorum);
    void set_dropout_quorum(uint64_t quorum);
    void set_quorum(uint64_t quorum);
};

} // namespace chain
} // namespace scorum
