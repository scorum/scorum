#include <scorum/chain/evaluators/proposal_create_evaluator.hpp>

#include <scorum/chain/evaluators/committee_accessor.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

proposal_create_evaluator::proposal_create_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, proposal_create_evaluator>(services)
    , _account_service(db().account_service())
    , _proposal_service(db().proposal_service())
    , _property_service(db().dynamic_global_property_service())
{
}

void proposal_create_evaluator::do_apply(const proposal_create_evaluator::operation_type& op)
{
    FC_ASSERT((op.lifetime_sec <= SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS
               && op.lifetime_sec >= SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS),
              "Proposal life time is not in range of ${min} - ${max} seconds.",
              ("min", SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS)("max", SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS));

    committee_i& committee_service = committee_accessor(db()).get_committee(op.operation);

    FC_ASSERT(committee_service.is_exists(op.creator), "Account \"${account_name}\" is not in committee.",
              ("account_name", op.creator));

    _account_service.check_account_existence(op.creator);

    const fc::time_point_sec expiration = _property_service.head_block_time() + op.lifetime_sec;

    const protocol::percent_type quorum = operation_get_required_quorum(committee_service, op.operation);

    _proposal_service.create(op.creator, op.operation, expiration, quorum);
}

} // namespace chain
} // namespace scorum
