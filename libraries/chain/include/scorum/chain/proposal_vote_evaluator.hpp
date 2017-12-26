#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluator.hpp>
#include <scorum/chain/dbservice.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/proposal_vote_object.hpp>
#include <scorum/chain/dbs_committee_proposal.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;

class dbservice;

template <typename EvaluatorType, typename OperationType = scorum::protocol::operation>
class evaluator_impl_x : public evaluator<OperationType>
{
public:
    typedef OperationType operation_sv_type;
    // typedef typename EvaluatorType::operation_type op_type;

    evaluator_impl_x()
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
};

// clang-format off
template <typename AccountService, typename ProposalService, typename CommiteeService>
class proposal_vote_evaluator_t : public scorum::chain::evaluator_impl_x<proposal_vote_evaluator_t<AccountService,
                                                                                                 ProposalService,
                                                                                                 CommiteeService>>
// clang-format on
{
public:
    typedef proposal_vote_operation operation_type;

    proposal_vote_evaluator_t(AccountService& account_service,
                              ProposalService& proposal_service,
                              CommiteeService& commitee_service,
                              uint32_t quorum)
        : scorum::chain::evaluator_impl_x<proposal_vote_evaluator_t>()
        , _account_service(account_service)
        , _proposal_service(proposal_service)
        , _committee_service(commitee_service)
        , _quorum(quorum)
    {
    }

    void do_apply(const proposal_vote_operation& op)
    {
        FC_ASSERT(_account_service.check_account_existence(op.voting_account),
                  "Account \"${account_name}\" must exist.", ("account_name", op.voting_account));

        FC_ASSERT(_proposal_service.is_exist(op.committee_member),
                  "There is no proposal for account name '${account_name}'", ("account_name", op.committee_member));

        const proposal_vote_object& proposal = _proposal_service.get(op.committee_member);

        FC_ASSERT(!_proposal_service.is_expired(proposal), "This proposal is expired '${account_name}'",
                  ("account_name", op.committee_member));

        _proposal_service.vote_for(proposal);

        size_t members_count = 10; // committee_service.get_members_count();

        if (check_quorum(proposal, _quorum, members_count))
        {
            if (proposal.action == invite)
            {
                _committee_service.add_member(op.committee_member);
            }
            else if (proposal.action == dropout)
            {
                _committee_service.exclude_member(op.committee_member);
            }
            else
            {
                FC_ASSERT("Invalid proposal action type");
            }

            _proposal_service.remove(proposal);
        }
    }

protected:
    AccountService& _account_service;
    ProposalService& _proposal_service;
    CommiteeService& _committee_service;

    uint32_t _quorum;
};

typedef proposal_vote_evaluator_t<dbs_account, dbs_committee_proposal, dbs_registration_committee>
    proposal_vote_evaluator;

} // namespace chain
} // namespace scorum
