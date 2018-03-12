#include <scorum/protocol/proposal_operations.hpp>
#include <scorum/protocol/operation_util_impl.hpp>

namespace scorum {
namespace protocol {

void operation_validate(const proposal_operation& op)
{
    op.visit(scorum::protocol::operation_validate_visitor());
}

percent_type operation_get_required_quorum(committee_i& committee_service, const proposal_operation& op)
{
    return op.visit(operation_get_required_quorum_visitor(committee_service));
}

void development_committee_transfer_operation::validate() const
{
    validate_account_name(to_account);

    FC_ASSERT(amount > asset(0, SCORUM_SYMBOL), "Must transfer a nonzero amount");
}

percent_type development_committee_transfer_operation::get_required_quorum(committee_i& committee_service) const
{
    return committee_service.get_transfer_quorum();
}

void development_committee_withdraw_vesting_operation::validate() const
{
    FC_ASSERT(vesting_shares > asset(0, SP_SYMBOL), "Must withdraw a nonzero amount");
}

percent_type development_committee_withdraw_vesting_operation::get_required_quorum(committee_i& committee_service) const
{
    return committee_service.get_transfer_quorum();
}

void development_committee_change_quorum_operation::validate() const
{
    validate_quorum(committee_quorum, quorum);
}

percent_type development_committee_change_quorum_operation::get_required_quorum(committee_i& committee_service) const
{
    return committee_service.get_base_quorum();
}

void development_committee_exclude_member_operation::validate() const
{
    validate_account_name(account_name);
}

percent_type development_committee_exclude_member_operation::get_required_quorum(committee_i& committee_service) const
{
    return committee_service.get_exclude_member_quorum();
}

void development_committee_add_member_operation::validate() const
{
    validate_account_name(account_name);
}

percent_type development_committee_add_member_operation::get_required_quorum(committee_i& committee_service) const
{
    return committee_service.get_add_member_quorum();
}

void registration_committee_change_quorum_operation::validate() const
{
    validate_quorum(committee_quorum, quorum);
}

percent_type registration_committee_change_quorum_operation::get_required_quorum(committee_i& committee_service) const
{
    return committee_service.get_base_quorum();
}

void registration_committee_exclude_member_operation::validate() const
{
    validate_account_name(account_name);
}

percent_type registration_committee_exclude_member_operation::get_required_quorum(committee_i& committee_service) const
{
    return committee_service.get_exclude_member_quorum();
}

void registration_committee_add_member_operation::validate() const
{
    validate_account_name(account_name);
}

percent_type registration_committee_add_member_operation::get_required_quorum(committee_i& committee_service) const
{
    return committee_service.get_add_member_quorum();
}

} // namespace protocol
} // namespace scorum

DEFINE_OPERATION_SERIALIZATOR(scorum::protocol::proposal_operation)
