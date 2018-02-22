#include <scorum/chain/evaluators/proposal_create_evaluator2.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

proposal_create_evaluator2::proposal_create_evaluator2(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, proposal_create_evaluator2>(services)
    , _account_service(db().account_service())
    , _proposal_service(db().proposal_service())
    , _committee_service(db().registration_committee_service())
    , _property_service(db().dynamic_global_property_service())
{
}

void proposal_create_evaluator2::do_apply(const proposal_create_evaluator2::operation_type& op)
{
    FC_ASSERT((op.lifetime_sec <= SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS
               && op.lifetime_sec >= SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS),
              "Proposal life time is not in range of ${min} - ${max} seconds.",
              ("min", SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS)("max", SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS));

    FC_ASSERT(_committee_service.is_exists(op.creator), "Account \"${account_name}\" is not in committee.",
              ("account_name", op.creator));

    _account_service.check_account_existence(op.creator);

    const fc::time_point_sec expiration = _property_service.head_block_time() + op.lifetime_sec;

    const protocol::percent_type quorum = SCORUM_COMMITTEE_QUORUM_PERCENT;

    _proposal_service.create2(op.creator, op.operation, expiration, quorum);
}

protocol::percent_type proposal_create_evaluator2::get_quorum(const change_quorum_operation& op) const
{
    return 0u;
}

} // namespace chain
} // namespace scorum
