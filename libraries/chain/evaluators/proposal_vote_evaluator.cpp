#include <scorum/chain/evaluators/proposal_vote_evaluator.hpp>

#include <scorum/chain/schema/proposal_object.hpp>

#include <scorum/chain/evaluators/committee_accessor.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/proposal_executor.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/proposal_executor.hpp>

namespace scorum {
namespace chain {

proposal_vote_evaluator::proposal_vote_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, proposal_vote_evaluator>(services)
    , _account_service(db().account_service())
    , _proposal_service(db().proposal_service())
    , _properties_service(db().dynamic_global_property_service())
    , _proposal_executor(db().proposal_executor_service())
{
}

void proposal_vote_evaluator::do_apply(const proposal_vote_evaluator::operation_type& op)
{
    FC_ASSERT(_proposal_service.is_exists(op.proposal_id), "There is no proposal with id '${id}'",
              ("id", op.proposal_id));

    const proposal_object& proposal = _proposal_service.get(op.proposal_id);

    auto committee = proposal.operation.visit(get_operation_committee_visitor(db()));

    FC_ASSERT(committee.as_committee_i().is_exists(op.voting_account),
              "Account \"${account_name}\" is not in committee.", ("account_name", op.voting_account));

    _account_service.check_account_existence(op.voting_account);

    FC_ASSERT(proposal.voted_accounts.find(op.voting_account) == proposal.voted_accounts.end(),
              "Account \"${account}\" already voted", ("account", op.voting_account));

    FC_ASSERT(!_proposal_service.is_expired(proposal), "Proposal '${id}' is expired.", ("id", op.proposal_id));

    _proposal_service.vote_for(op.voting_account, proposal);

    _proposal_executor(proposal);
}

} // namespace chain
} // namespace scorum
