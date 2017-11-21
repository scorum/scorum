#pragma once

#include <memory>

#include <scorum/chain/dbs_base_impl.hpp>

namespace scorum {
namespace chain {

class witness_object;
class witness_schedule_object;

class dbs_witness : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_witness(database& db);

public:

    const witness_object& get_witness(const account_name_type& name) const;

    const witness_schedule_object& get_witness_schedule_object() const;

    const witness_object& get_top_witness() const;

};
}
}
