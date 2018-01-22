#pragma once

#include <scorum/chain/services/base_service.hpp>

#include <scorum/chain/witness_objects.hpp>

namespace scorum {
namespace chain {

struct witness_vote_service_i
{
    virtual void create(witness_id_type witness_id, account_id_type voter_id) = 0;
    virtual bool is_exists(witness_id_type witness_id, account_id_type voter_id) const = 0;
    virtual const witness_vote_object& get(witness_id_type witness_id, account_id_type voter_id) = 0;
    virtual void remove(const witness_vote_object& witness_vote) = 0;
};

class dbs_witness_vote : public dbs_base, public witness_vote_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_witness_vote(database& db);

public:
    virtual void create(witness_id_type witness_id, account_id_type voter_id) override;
    virtual bool is_exists(witness_id_type witness_id, account_id_type voter_id) const override;
    virtual const witness_vote_object& get(witness_id_type witness_id, account_id_type voter_id) override;
    virtual void remove(const witness_vote_object& witness_vote) override;
};

} // namespace chain
} // namespace scorum
