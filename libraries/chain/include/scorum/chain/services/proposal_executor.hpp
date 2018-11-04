#pragma once

#include <scorum/chain/services/dbs_base.hpp>
#include <scorum/chain/evaluators/evaluator_registry.hpp>
#include <scorum/protocol/proposal_operations.hpp>

namespace scorum {
namespace chain {

struct data_service_factory_i;
struct proposal_service_i;
class proposal_object;

struct proposal_executor_service_i
{
    virtual void operator()(const proposal_object& proposal) = 0;
};

class dbs_proposal_executor : public dbs_base, public proposal_executor_service_i
{
    friend class dbservice_dbs_factory;

public:
    explicit dbs_proposal_executor(database& s);

    void operator()(const proposal_object& proposal) override;

private:
    void execute_proposal(const proposal_object& proposal);
    void update_proposals_voting_list_and_execute();
    bool is_quorum(const proposal_object& proposal);

    data_service_factory_i& services;
    proposal_service_i& proposal_service;
    evaluator_registry<protocol::proposal_operation> evaluators;
    fc::flat_set<account_name_type> removed_members;
    // TODO: tech dept: remove this dependency if possible
    database& _db;
};

} // namespace scorum
} // namespace chain
