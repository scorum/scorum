#include <scorum/chain/evaluators/proposal_create_evaluator.hpp>

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
    , _committee_service(db().registration_committee_service())
    , _property_service(db().dynamic_global_property_service())
{
}

void proposal_create_evaluator::do_apply(const proposal_create_evaluator::operation_type& op)
{
    FC_ASSERT((op.lifetime_sec <= SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS
               && op.lifetime_sec >= SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS),
              "Proposal life time is not in range of ${min} - ${max} seconds.",
              ("min", SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS)("max", SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS));

    FC_ASSERT(_committee_service.is_exists(op.creator), "Account \"${account_name}\" is not in committee.",
              ("account_name", op.creator));

    _account_service.check_account_existence(op.creator);

    fc::time_point_sec expiration = _property_service.head_block_time() + op.lifetime_sec;

    _proposal_service.create(op.creator, op.data, *op.action, expiration, get_quorum(*op.action));
}

uint64_t proposal_create_evaluator::get_quorum(scorum::protocol::proposal_action action)
{
    using proposal_action = scorum::protocol::proposal_action;

    const dynamic_global_property_object& properties = _property_service.get_dynamic_global_properties();

    switch (action)
    {
    case proposal_action::invite:
        return properties.invite_quorum;

    case proposal_action::dropout:
        return properties.dropout_quorum;

    case proposal_action::change_invite_quorum:
    case proposal_action::change_dropout_quorum:
    case proposal_action::change_quorum:
        return properties.change_quorum;

    default:
        FC_ASSERT("invalid action type.");
    }

    return SCORUM_COMMITTEE_QUORUM_PERCENT;
}

} // namespace chain
} // namespace scorum
