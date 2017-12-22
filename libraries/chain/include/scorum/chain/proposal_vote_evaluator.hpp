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

// clang-format off
template <typename AccountService, typename ProposalService, typename CommiteeService>
class proposal_vote_evaluator_t : public scorum::chain::evaluator_impl<proposal_vote_evaluator_t<AccountService,
                                                                                                 ProposalService,
                                                                                                 CommiteeService>>
// clang-format on
{
public:
    typedef proposal_vote_operation operation_type;

    proposal_vote_evaluator_t(dbservice& db, uint32_t quorum)
        : scorum::chain::evaluator_impl<proposal_vote_evaluator_t>(db)
        , account_service(db.obtain_service<AccountService>())
        , proposal_service(db.obtain_service<ProposalService>())
        , committee_service(db.obtain_service<CommiteeService>())
        , _quorum(quorum)
    {
    }

    void do_apply(const proposal_vote_operation& op)
    {
        account_service.check_account_existence(op.voting_account);

        //        const proposal_vote_object& proposal = proposal_service.get(op.committee_member);
        // proposal_service.check_expiration(proposal);

        const proposal_vote_object& proposal = proposal_service.vote_for(op.committee_member);

        size_t members_count = 10; // committee_service.get_members_count();

        if (check_quorum(proposal, _quorum, members_count))
        {
            if (proposal.action == invite)
            {
                committee_service.add_member(op.committee_member);
            }
            else if (proposal.action == dropout)
            {
                committee_service.exclude_member(op.committee_member);
            }
            else
            {
                FC_ASSERT("Invalid proposal action type");
            }

            proposal_service.remove(proposal);
        }
    }

protected:
    AccountService& account_service;
    ProposalService& proposal_service;
    CommiteeService& committee_service;

    uint32_t _quorum;
};

typedef proposal_vote_evaluator_t<dbs_account, dbs_committee_proposal, dbs_registration_committee>
    proposal_vote_evaluator;

} // namespace chain
} // namespace scorum
