#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

#include <scorum/protocol/types.hpp>

#include <scorum/chain/schema/proposal_object.hpp>

namespace scorum {
namespace chain {

class account_service_i;
class proposal_service_i;
class registration_committee_service_i;
class dynamic_global_property_service_i;

class data_service_factory_i;

class proposal_object;

using proposal_action = scorum::protocol::proposal_action;

class proposal_vote_evaluator2 : public evaluator_impl<data_service_factory_i, proposal_vote_evaluator2>
{
public:
    using operation_type = scorum::protocol::proposal_vote_operation;

    proposal_vote_evaluator2(data_service_factory_i& services);

    using evaluator_callback = std::function<void(const proposal_object&)>;

    class proposal_evaluators_register
    {
    public:
        void set(proposal_action action, evaluator_callback callback)
        {
            _register.insert(std::make_pair(action, callback));
        }

        void execute(const proposal_object& proposal)
        {
            if (_register.count(proposal.action) == 0)
            {
                FC_ASSERT("Invalid proposal action type");
            }
            else
            {
                _register[proposal.action](proposal);
            }
        }

    private:
        fc::flat_map<proposal_action, evaluator_callback> _register;
    };

    void do_apply(const operation_type& op);

protected:
    virtual void update_proposals_voting_list_and_execute();

    virtual void execute_proposal(const proposal_object& proposal);

    bool is_quorum(const proposal_object& proposal);

    void invite_evaluator(const proposal_object& proposal);

    void dropout_evaluator(const proposal_object& proposal);

    void change_invite_quorum_evaluator(const proposal_object& proposal);

    void change_dropout_quorum_evaluator(const proposal_object& proposal);

    void change_quorum_evaluator(const proposal_object& proposal);

    account_service_i& _account_service;
    proposal_service_i& _proposal_service;
    registration_committee_service_i& _committee_service;
    dynamic_global_property_service_i& _properties_service;

    fc::flat_set<account_name_type> removed_members;

private:
    proposal_evaluators_register evaluators;
};

} // namespace chain
} // namespace scorum
