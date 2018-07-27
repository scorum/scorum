#include <scorum/protocol/proposal_operations.hpp>
#include <scorum/protocol/operation_util_impl.hpp>

namespace scorum {
namespace protocol {

committee_i& committee::as_committee_i() &
{
    return visit([](registration_committee_i& c) { return utils::make_ref(static_cast<committee_i&>(c)); },
                 [](development_committee_i& c) { return utils::make_ref(static_cast<committee_i&>(c)); });
}

void development_committee_transfer_operation::validate() const
{
    validate_account_name(to_account);

    FC_ASSERT(amount > asset(0, SCORUM_SYMBOL), "Must transfer a nonzero amount");
}

percent_type development_committee_transfer_operation::get_required_quorum(committee_type& committee_service) const
{
    return committee_service.get_transfer_quorum();
}

void development_committee_withdraw_vesting_operation::validate() const
{
    FC_ASSERT(vesting_shares > asset(0, SP_SYMBOL), "Must withdraw a nonzero amount");
}

percent_type
development_committee_withdraw_vesting_operation::get_required_quorum(committee_type& committee_service) const
{
    return committee_service.get_transfer_quorum();
}

void development_committee_change_quorum_operation::validate() const
{
    validate_quorum(committee_quorum, quorum);
}

percent_type development_committee_change_quorum_operation::get_required_quorum(committee_type& committee_service) const
{
    return committee_service.get_base_quorum();
}

void development_committee_exclude_member_operation::validate() const
{
    validate_account_name(account_name);
}

percent_type
development_committee_exclude_member_operation::get_required_quorum(committee_type& committee_service) const
{
    return committee_service.get_exclude_member_quorum();
}

void development_committee_add_member_operation::validate() const
{
    validate_account_name(account_name);
}

percent_type development_committee_add_member_operation::get_required_quorum(committee_type& committee_service) const
{
    return committee_service.get_add_member_quorum();
}

void development_committee_empower_advertising_moderator_operation::validate() const
{
    validate_account_name(account);
}

percent_type development_committee_empower_advertising_moderator_operation::get_required_quorum(
    committee_type& committee_service) const
{
    return committee_service.get_advertising_moderator_quorum();
}

void development_committee_empower_betting_moderator_operation::validate() const
{
    validate_account_name(account);
}

percent_type
development_committee_empower_betting_moderator_operation::get_required_quorum(committee_type& committee_service) const
{
    return committee_service.get_betting_moderator_quorum();
}

void registration_committee_change_quorum_operation::validate() const
{
    validate_quorum(committee_quorum, quorum);
}

percent_type
registration_committee_change_quorum_operation::get_required_quorum(committee_type& committee_service) const
{
    return committee_service.get_base_quorum();
}

void registration_committee_exclude_member_operation::validate() const
{
    validate_account_name(account_name);
}

percent_type
registration_committee_exclude_member_operation::get_required_quorum(committee_type& committee_service) const
{
    return committee_service.get_exclude_member_quorum();
}

void registration_committee_add_member_operation::validate() const
{
    validate_account_name(account_name);
}

percent_type registration_committee_add_member_operation::get_required_quorum(committee_type& committee_service) const
{
    return committee_service.get_add_member_quorum();
}

void base_development_committee_change_budgets_vcg_properties_operation::validate() const
{
    FC_ASSERT(vcg_coefficients.size() > 1u, "Invalid coefficient's list");
    FC_ASSERT((*vcg_coefficients.begin()) <= 100, "Invalid coefficient's list");
    FC_ASSERT((*vcg_coefficients.rbegin()) > 0, "Invalid coefficient's list");
    FC_ASSERT(std::is_sorted(vcg_coefficients.rbegin(), vcg_coefficients.rend()), "Invalid coefficient's list");
}

percent_type base_development_committee_change_budgets_vcg_properties_operation::get_required_quorum(
    committee_type& committee_service) const
{
    return committee_service.get_budgets_vcg_properties_quorum();
}

struct operation_get_required_quorum_visitor
{
    typedef scorum::protocol::percent_type result_type;

    operation_get_required_quorum_visitor(committee& committee)
        : _committee(committee)
    {
    }

    template <typename T> protocol::percent_type operator()(const T& op) const
    {
        auto& committee = _committee.get<utils::ref<typename T::committee_type>>();
        return op.get_required_quorum(committee);
    }

private:
    committee& _committee;
};

void operation_validate(const proposal_operation& op)
{
    op.visit(scorum::protocol::operation_validate_visitor());
}

percent_type operation_get_required_quorum(committee& committee_service, const proposal_operation& op)
{
    return op.visit(operation_get_required_quorum_visitor(committee_service));
}
} // namespace protocol
} // namespace scorum

DEFINE_OPERATION_SERIALIZATOR(scorum::protocol::proposal_operation)
