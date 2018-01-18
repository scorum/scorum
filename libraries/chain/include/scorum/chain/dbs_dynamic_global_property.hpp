#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

namespace scorum {
namespace chain {

class dynamic_global_property_object;
class database;

class property_service_i
{
public:
    virtual const dynamic_global_property_object& get_dynamic_global_properties() const = 0;
    virtual fc::time_point_sec head_block_time() = 0;
};

class dbs_dynamic_global_property : public property_service_i, public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_dynamic_global_property(database& db);

public:
    virtual const dynamic_global_property_object& get_dynamic_global_properties() const;

    virtual fc::time_point_sec head_block_time();

    void set_invite_quorum(uint64_t quorum);
    void set_dropout_quorum(uint64_t quorum);
    void set_quorum(uint64_t quorum);
};

} // namespace chain
} // namespace scorum
