#include <scorum/chain/evaluators/proposal_vote_evaluator2.hpp>

#include <functional>

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/proposal_object.hpp>

namespace scorum {
namespace chain {

proposal_vote_evaluator2::proposal_vote_evaluator2(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, proposal_vote_evaluator2>(services)
    , _account_service(db().account_service())
    , _proposal_service(db().proposal_service())
    , _committee_service(db().registration_committee_service())
    , _properties_service(db().dynamic_global_property_service())
{
    evaluators.set(proposal_action::invite,
                   std::bind(&proposal_vote_evaluator2::invite_evaluator, this, std::placeholders::_1));
    evaluators.set(proposal_action::dropout,
                   std::bind(&proposal_vote_evaluator2::dropout_evaluator, this, std::placeholders::_1));
    evaluators.set(proposal_action::change_invite_quorum,
                   std::bind(&proposal_vote_evaluator2::change_invite_quorum_evaluator, this, std::placeholders::_1));
    evaluators.set(proposal_action::change_dropout_quorum,
                   std::bind(&proposal_vote_evaluator2::change_dropout_quorum_evaluator, this, std::placeholders::_1));
    evaluators.set(proposal_action::change_quorum,
                   std::bind(&proposal_vote_evaluator2::change_quorum_evaluator, this, std::placeholders::_1));
}

void proposal_vote_evaluator2::do_apply(const proposal_vote_evaluator2::operation_type& op)
{
    FC_ASSERT(_committee_service.is_exists(op.voting_account), "Account \"${account_name}\" is not in committee.",
              ("account_name", op.voting_account));

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

void proposal_vote_evaluator2::update_proposals_voting_list_and_execute()
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

void proposal_vote_evaluator2::execute_proposal(const proposal_object& proposal)
{
    if (is_quorum(proposal))
    {
        evaluators.execute(proposal);
        _proposal_service.remove(proposal);
    }
}

bool proposal_vote_evaluator2::is_quorum(const proposal_object& proposal)
{
    const size_t votes = _proposal_service.get_votes(proposal);
    const size_t members_count = _committee_service.get_members_count();

    return utils::is_quorum(votes, members_count, proposal.quorum_percent);
}

void proposal_vote_evaluator2::invite_evaluator(const proposal_object& proposal)
{
    account_name_type member = proposal.data.as_string();
    _committee_service.add_member(member);
}

void proposal_vote_evaluator2::dropout_evaluator(const proposal_object& proposal)
{
    account_name_type member = proposal.data.as_string();
    _committee_service.exclude_member(member);
    removed_members.insert(member);
}

void proposal_vote_evaluator2::change_invite_quorum_evaluator(const proposal_object& proposal)
{
    uint64_t quorum = proposal.data.as_uint64();
    _properties_service.set_invite_quorum(quorum);
}

void proposal_vote_evaluator2::change_dropout_quorum_evaluator(const proposal_object& proposal)
{
    uint64_t quorum = proposal.data.as_uint64();
    _properties_service.set_dropout_quorum(quorum);
}

void proposal_vote_evaluator2::change_quorum_evaluator(const proposal_object& proposal)
{
    uint64_t quorum = proposal.data.as_uint64();
    _properties_service.set_quorum(quorum);
}

} // namespace chain
} // namespace scorum
