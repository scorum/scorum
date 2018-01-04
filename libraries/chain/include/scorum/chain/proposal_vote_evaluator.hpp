#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluator.hpp>
#include <scorum/chain/dbservice.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>

#include <scorum/chain/proposal_vote_object.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;

// clang-format off
template <typename AccountService,
          typename ProposalService,
          typename CommitteeService,
          typename OperationType = scorum::protocol::operation>
class proposal_vote_evaluator_t : public evaluator<OperationType>
// clang-format on
{
public:
    //    typedef scorum::protocol::operation ;
    typedef proposal_vote_evaluator_t<AccountService, ProposalService, CommitteeService> EvaluatorType;
    typedef proposal_vote_operation operation_type;

    proposal_vote_evaluator_t(AccountService& account_service,
                              ProposalService& proposal_service,
                              CommitteeService& committee_service)
        : _account_service(account_service)
        , _proposal_service(proposal_service)
        , _committee_service(committee_service)
    {
    }

    virtual void apply(const OperationType& o) final override
    {
        auto* eval = static_cast<EvaluatorType*>(this);
        const auto& op = o.template get<typename EvaluatorType::operation_type>();
        eval->do_apply(op);
    }

    virtual int get_type() const override
    {
        return OperationType::template tag<typename EvaluatorType::operation_type>::value;
    }

    void do_apply(const proposal_vote_operation& op)
    {
        FC_ASSERT(_committee_service.member_exists(op.voting_account),
                  "Account \"${account_name}\" is not in committee.", ("account_name", op.voting_account));

        FC_ASSERT(_account_service.is_exists(op.voting_account), "Account \"${account_name}\" must exist.",
                  ("account_name", op.voting_account));

        FC_ASSERT(_proposal_service.is_exist(op.proposal_id), "There is no proposal with id '${id}'",
                  ("id", op.proposal_id));

        const proposal_vote_object& proposal = _proposal_service.get(op.proposal_id);

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

            auto updated_proposals = _proposal_service.for_all_proposals_remove_from_voting_list(member);

            for (auto p : updated_proposals)
            {
                execute_proposal(p);
            }

            removed_members.erase(member);
        }
    }

    virtual void execute_proposal(const proposal_vote_object& proposal)
    {
        const size_t votes = _proposal_service.get_votes(proposal);
        const size_t needed_votes = _committee_service.quorum_votes(proposal.quorum_percent);

        if (votes >= needed_votes)
        {
            if (proposal.action == invite)
            {
                _committee_service.add_member(proposal.member);
            }
            else if (proposal.action == dropout)
            {
                _committee_service.exclude_member(proposal.member);
                removed_members.insert(proposal.member);
            }
            else
            {
                FC_ASSERT("Invalid proposal action type");
            }

            _proposal_service.remove(proposal);
        }
    }

    AccountService& _account_service;
    ProposalService& _proposal_service;
    CommitteeService& _committee_service;

    flat_set<account_name_type> removed_members;
};

typedef proposal_vote_evaluator_t<dbs_account, dbs_proposal, dbs_registration_committee> proposal_vote_evaluator;

} // namespace chain
} // namespace scorum
