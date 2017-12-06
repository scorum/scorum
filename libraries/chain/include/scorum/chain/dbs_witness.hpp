#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

namespace scorum {
namespace chain {

class witness_object;
class witness_schedule_object;
class account_object;

class dbs_witness : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_witness(database& db);

public:
    const witness_object& get_witness(const account_name_type& name) const;

    const witness_schedule_object& get_witness_schedule_object() const;

    const witness_object& get_top_witness() const;

    /** this updates the vote of a single witness as a result of a vote being added or removed*/
    void adjust_witness_vote(const witness_object& witness, share_type delta);

    /** this is called by `adjust_proxied_witness_votes` when account proxy to self */
    void adjust_witness_votes(const account_object& account, share_type delta);
};
}
}
