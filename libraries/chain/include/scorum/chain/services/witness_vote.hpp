#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

namespace scorum {
namespace chain {

struct witness_vote_service_i : public base_service_i<witness_vote_object>
{
    using base_service_i<witness_vote_object>::get;
    using base_service_i<witness_vote_object>::is_exists;

    virtual bool is_exists(witness_id_type witness_id, account_id_type voter_id) const = 0;
    virtual const witness_vote_object& get(witness_id_type witness_id, account_id_type voter_id) = 0;
};

class dbs_witness_vote : public dbs_service_base<witness_vote_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_witness_vote(database& db);

public:
    using base_service_i<witness_vote_object>::get;
    using base_service_i<witness_vote_object>::is_exists;

    virtual bool is_exists(witness_id_type witness_id, account_id_type voter_id) const override;
    virtual const witness_vote_object& get(witness_id_type witness_id, account_id_type voter_id) override;
};

} // namespace chain
} // namespace scorum
