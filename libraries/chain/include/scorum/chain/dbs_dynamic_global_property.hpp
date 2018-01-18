#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

namespace scorum {
namespace chain {

class dynamic_global_property_object;

class dbs_dynamic_global_property : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_dynamic_global_property(database& db);

public:
    const dynamic_global_property_object& get_dynamic_global_properties() const;

    time_point_sec head_block_time() const;

    void set_invite_quorum(uint64_t quorum);
    void set_dropout_quorum(uint64_t quorum);
    void set_quorum(uint64_t quorum);
};

} // namespace scorum
} // namespace chain
