#pragma once

#include <functional>

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/global_property_object.hpp>

#include <scorum/chain/evaluator.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/dbs_dynamic_global_property.hpp>

#include <scorum/chain/proposal_object.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;

// clang-format off
template <typename AccountService,
          typename ProposalService,
          typename CommitteeService,
          typename PropertiesService,
          typename OperationType = scorum::protocol::operation>
class proposal_vote_evaluator_t : public evaluator<OperationType>
// clang-format on
{
public:
    typedef proposal_vote_operation operation_type;
    typedef proposal_vote_evaluator_t<AccountService,
                                      ProposalService,
                                      CommitteeService,
                                      PropertiesService,
                                      OperationType>
        EvaluatorType;

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

    proposal_vote_evaluator_t(AccountService& account_service,
                              ProposalService& proposal_service,
                              CommitteeService& committee_service,
                              PropertiesService& pool_service)
        : _account_service(account_service)
        , _proposal_service(proposal_service)
        , _committee_service(committee_service)
        , _properties_service(pool_service)
    {
        evaluators.set(proposal_action::invite,
                       std::bind(&EvaluatorType::invite_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action::dropout,
                       std::bind(&EvaluatorType::dropout_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action::change_invite_quorum,
                       std::bind(&EvaluatorType::change_invite_quorum_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action::change_dropout_quorum,
                       std::bind(&EvaluatorType::change_dropout_quorum_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action::change_quorum,
                       std::bind(&EvaluatorType::change_quorum_evaluator, this, std::placeholders::_1));
    }

    virtual void apply(const OperationType& o) final override
    {
        const auto& op = o.template get<operation_type>();
        this->do_apply(op);
    }

    virtual int get_type() const override
    {
        return OperationType::template tag<operation_type>::value;
    }

    void do_apply(const proposal_vote_operation& op)
    {
        FC_ASSERT(_committee_service.member_exists(op.voting_account),
                  "Account \"${account_name}\" is not in committee.", ("account_name", op.voting_account));

        _account_service.check_account_existence(op.voting_account);

        FC_ASSERT(_proposal_service.is_exists(op.proposal_id), "There is no proposal with id '${id}'",
                  ("id", op.proposal_id));

        const proposal_object& proposal = _proposal_service.get(op.proposal_id);

        FC_ASSERT(proposal.voted_accounts.find(op.voting_account) == proposal.voted_accounts.end(),
                  "Account \"${account}\" already voted", ("account", op.voting_account));

        FC_ASSERT(!_proposal_service.is_expired(proposal), "Proposal '${id}' is expired.", ("id", op.proposal_id));

        _proposal_service.vote_for(op.voting_account, proposal);

        execute_proposal(proposal);

        update_proposals_voting_list_and_execute();
    }

protected:
    virtual void update_proposals_voting_list_and_execute()
    {
        while (!removed_members.empty())
        {
            account_name_type member = *removed_members.begin();

            _proposal_service.for_all_proposals_remove_from_voting_list(member);

            auto proposals = _proposal_service.get_proposals();

            for (const auto& p : proposals)
            {
                execute_proposal(p);
            }

            removed_members.erase(member);
        }
    }

    virtual void execute_proposal(const proposal_object& proposal)
    {
        if (is_quorum(proposal))
        {
            evaluators.execute(proposal);
            _proposal_service.remove(proposal);
        }
    }

    bool is_quorum(const proposal_object& proposal)
    {
        const size_t votes = _proposal_service.get_votes(proposal);
        const size_t needed_votes = _committee_service.quorum_votes(proposal.quorum_percent);

        return votes >= needed_votes ? true : false;
    }

    void invite_evaluator(const proposal_object& proposal)
    {
        account_name_type member = proposal.data.as_string();
        _committee_service.add_member(member);
    }

    void dropout_evaluator(const proposal_object& proposal)
    {
        account_name_type member = proposal.data.as_string();
        _committee_service.exclude_member(member);
        removed_members.insert(member);
    }

    void change_invite_quorum_evaluator(const proposal_object& proposal)
    {
        uint64_t quorum = proposal.data.as_uint64();
        _properties_service.set_invite_quorum(quorum);
    }

    void change_dropout_quorum_evaluator(const proposal_object& proposal)
    {
        uint64_t quorum = proposal.data.as_uint64();
        _properties_service.set_dropout_quorum(quorum);
    }

    void change_quorum_evaluator(const proposal_object& proposal)
    {
        uint64_t quorum = proposal.data.as_uint64();
        _properties_service.set_quorum(quorum);
    }

    AccountService& _account_service;
    ProposalService& _proposal_service;
    CommitteeService& _committee_service;
    PropertiesService& _properties_service;

    fc::flat_set<account_name_type> removed_members;

private:
    proposal_evaluators_register evaluators;
};

typedef proposal_vote_evaluator_t<dbs_account, dbs_proposal, dbs_registration_committee, dbs_dynamic_global_property>
    proposal_vote_evaluator;

} // namespace chain
} // namespace scorum
