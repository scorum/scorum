#include <scorum/chain/evaluators/proposal_create_evaluator2.hpp>

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

struct committee_accessor
{
    explicit committee_accessor(data_service_factory_i& services)
        : _services(services)
    {
    }

    template <typename T> protocol::committee& get_committee(const T&) const
    {
        FC_THROW_EXCEPTION(fc::assert_exception, "This operation is not implemented");
    }

private:
    data_service_factory_i& _services;
};

template <>
protocol::committee&
committee_accessor::get_committee(const proposal_committee_operation<registration_committee>&) const
{
    return _services.registration_committee_service();
}

proposal_create_evaluator2::proposal_create_evaluator2(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, proposal_create_evaluator2>(services)
    , _account_service(db().account_service())
    , _proposal_service(db().proposal_service())
    , _property_service(db().dynamic_global_property_service())
{
}

void proposal_create_evaluator2::do_apply(const proposal_create_evaluator2::operation_type& op)
{
    FC_ASSERT((op.lifetime_sec <= SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS
               && op.lifetime_sec >= SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS),
              "Proposal life time is not in range of ${min} - ${max} seconds.",
              ("min", SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS)("max", SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS));

    protocol::committee& committee_service = get_committee(op.operation);

    FC_ASSERT(committee_service.is_exists(op.creator), "Account \"${account_name}\" is not in committee.",
              ("account_name", op.creator));

    _account_service.check_account_existence(op.creator);

    const fc::time_point_sec expiration = _property_service.head_block_time() + op.lifetime_sec;

    const protocol::percent_type quorum = get_quorum(op.operation);

    _proposal_service.create2(op.creator, op.operation, expiration, quorum);
}

protocol::percent_type proposal_create_evaluator2::get_quorum(const protocol::proposal_operation& op)
{
    committee& c = get_committee(op);
    if (op.which() == proposal_operation::tag<registration_committee_add_member_operation>::value)
    {
        return c.get_add_member_quorum();
    }
    else if (op.which() == proposal_operation::tag<registration_committee_exclude_member_operation>::value)
    {
        return c.get_exclude_member_quorum();
    }
    else if (op.which() == proposal_operation::tag<registration_committee_change_quorum_operation>::value)
    {
        return c.get_base_quorum();
    }
    else
    {
        FC_THROW_EXCEPTION(fc::assert_exception, "Unknow operation.");
    }
}

committee& proposal_create_evaluator2::get_committee(const proposal_operation& op)
{
    using protocol::to_committee_operation;

    if (op.which() == proposal_operation::tag<registration_committee_add_member_operation>::value)
    {
        const auto& o = to_committee_operation().cast<registration_committee_add_member_operation>(op);

        return committee_accessor(db()).get_committee(o);
    }
    else if (op.which() == proposal_operation::tag<registration_committee_exclude_member_operation>::value)
    {
        const auto& o = to_committee_operation().cast<registration_committee_exclude_member_operation>(op);

        return committee_accessor(db()).get_committee(o);
    }
    else if (op.which() == proposal_operation::tag<registration_committee_change_quorum_operation>::value)
    {
        const auto& o = to_committee_operation().cast<registration_committee_change_quorum_operation>(op);

        return committee_accessor(db()).get_committee(o);
    }
    else
    {
        FC_THROW_EXCEPTION(fc::assert_exception, "Unknow operation.");
    }
}

} // namespace chain
} // namespace scorum
