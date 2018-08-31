#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>
#include <scorum/chain/evaluators/evaluator_registry.hpp>
#include <scorum/chain/evaluators/committee_accessor.hpp>

namespace scorum {
namespace chain {

struct data_service_factory_i;

template <typename EvaluatorType>
using proposal_operation_evaluator
    = scorum::chain::evaluator_impl<data_service_factory_i, EvaluatorType, protocol::proposal_operation>;

template <typename OperationType>
struct proposal_add_member_evaluator : public proposal_operation_evaluator<proposal_add_member_evaluator<OperationType>>
{
    typedef OperationType operation_type;

    proposal_add_member_evaluator(data_service_factory_i& r)
        : proposal_operation_evaluator<proposal_add_member_evaluator<OperationType>>(r)
    {
    }

    void do_apply(const operation_type& o)
    {
        auto committee = get_committee(this->db(), o);
        committee.as_committee_i().add_member(o.account_name);
    }
};

template <typename OperationType>
struct proposal_exclude_member_evaluator
    : public proposal_operation_evaluator<proposal_exclude_member_evaluator<OperationType>>
{
    typedef OperationType operation_type;

    proposal_exclude_member_evaluator(data_service_factory_i& s, fc::flat_set<account_name_type>& members)
        : proposal_operation_evaluator<proposal_exclude_member_evaluator<OperationType>>(s)
        , removed_members(members)
    {
    }

    void do_apply(const operation_type& o)
    {
        auto committee = get_committee(this->db(), o);
        committee.as_committee_i().exclude_member(o.account_name);

        removed_members.insert(o.account_name);
    }

private:
    fc::flat_set<account_name_type>& removed_members;
};

template <typename OperationType>
struct proposal_change_quorum_evaluator
    : public proposal_operation_evaluator<proposal_change_quorum_evaluator<OperationType>>
{
    typedef OperationType operation_type;

    proposal_change_quorum_evaluator(data_service_factory_i& r)
        : proposal_operation_evaluator<proposal_change_quorum_evaluator<OperationType>>(r)
    {
    }

    void do_apply(const operation_type& o)
    {
        committee committee = get_committee(this->db(), o);

        switch (o.committee_quorum)
        {
        case quorum_type::add_member_quorum:
            committee.as_committee_i().change_add_member_quorum(o.quorum);
            break;
        case quorum_type::exclude_member_quorum:
            committee.as_committee_i().change_exclude_member_quorum(o.quorum);
            break;
        case quorum_type::base_quorum:
            committee.as_committee_i().change_base_quorum(o.quorum);
            break;
        case quorum_type::transfer_quorum:
            committee.weak_visit([&](development_committee_i& c) { c.change_transfer_quorum(o.quorum); });
            break;
        case quorum_type::advertising_moderator_quorum:
            committee.weak_visit([&](development_committee_i& c) { c.change_advertising_moderator_quorum(o.quorum); });
            break;
        case quorum_type::betting_moderator_quorum:
            committee.weak_visit([&](development_committee_i& c) { c.change_betting_moderator_quorum(o.quorum); });
            break;
        case quorum_type::betting_resolve_delay_quorum:
            committee.weak_visit([&](development_committee_i& c) { c.change_betting_resolve_delay_quorum(o.quorum); });
            break;
        case quorum_type::budgets_vcg_properties_quorum:
            committee.weak_visit([&](development_committee_i& c) { c.change_budgets_vcg_properties_quorum(o.quorum); });
            break;
        case quorum_type::none_quorum:
            FC_THROW_EXCEPTION(fc::assert_exception, "unknow quorum change operation");
        }
    }
};

struct development_committee_withdraw_vesting_evaluator
    : public proposal_operation_evaluator<development_committee_withdraw_vesting_evaluator>
{
    typedef development_committee_withdraw_vesting_operation operation_type;

    development_committee_withdraw_vesting_evaluator(data_service_factory_i& r);

    void do_apply(const operation_type& o);
};

struct development_committee_transfer_evaluator
    : public proposal_operation_evaluator<development_committee_transfer_evaluator>
{
    typedef development_committee_transfer_operation operation_type;

    development_committee_transfer_evaluator(data_service_factory_i& r);

    void do_apply(const operation_type& o);
};

struct development_committee_empower_advertising_moderator_evaluator
    : public proposal_operation_evaluator<development_committee_empower_advertising_moderator_evaluator>
{
    typedef development_committee_empower_advertising_moderator_operation operation_type;

    development_committee_empower_advertising_moderator_evaluator(data_service_factory_i& r);

    void do_apply(const operation_type& o);
};

struct development_committee_empower_betting_moderator_evaluator
    : public proposal_operation_evaluator<development_committee_empower_betting_moderator_evaluator>
{
    typedef development_committee_empower_betting_moderator_operation operation_type;

    development_committee_empower_betting_moderator_evaluator(data_service_factory_i& r);

    void do_apply(const operation_type& o);
};

struct development_committee_change_betting_resolve_delay_evaluator
    : public proposal_operation_evaluator<development_committee_change_betting_resolve_delay_evaluator>
{
    typedef development_committee_change_betting_resolve_delay_operation operation_type;

    development_committee_change_betting_resolve_delay_evaluator(data_service_factory_i& r);

    void do_apply(const operation_type& o);
};

template <budget_type type>
struct development_committee_change_budgets_vcg_properties_evaluator
    : public proposal_operation_evaluator<development_committee_change_budgets_vcg_properties_evaluator<type>>
{
    using operation_type = development_committee_change_budgets_vcg_properties_operation<type>;
    using base_type = proposal_operation_evaluator<development_committee_change_budgets_vcg_properties_evaluator<type>>;

    development_committee_change_budgets_vcg_properties_evaluator(data_service_factory_i& r)
        : base_type(r)
    {
    }

    void do_apply(const operation_type& o);
};

using development_committee_change_top_post_budgets_amount_evaluator
    = development_committee_change_budgets_vcg_properties_evaluator<budget_type::post>;

using development_committee_change_top_banner_budgets_amount_evaluator
    = development_committee_change_budgets_vcg_properties_evaluator<budget_type::banner>;

namespace registration_committee {

using proposal_add_member_evaluator
    = scorum::chain::proposal_add_member_evaluator<registration_committee_add_member_operation>;

using proposal_exclude_member_evaluator
    = scorum::chain::proposal_exclude_member_evaluator<registration_committee_exclude_member_operation>;

using proposal_change_quorum_evaluator
    = scorum::chain::proposal_change_quorum_evaluator<registration_committee_change_quorum_operation>;

} // namespace registration_committee

namespace development_committee {

using proposal_add_member_evaluator
    = scorum::chain::proposal_add_member_evaluator<development_committee_add_member_operation>;

using proposal_exclude_member_evaluator
    = scorum::chain::proposal_exclude_member_evaluator<development_committee_exclude_member_operation>;

using proposal_change_quorum_evaluator
    = scorum::chain::proposal_change_quorum_evaluator<development_committee_change_quorum_operation>;

using proposal_withdraw_vesting_evaluator = development_committee_withdraw_vesting_evaluator;
using proposal_transfer_evaluator = development_committee_transfer_evaluator;

} // namespace development_committee

} // namespace chain
} // namespace scorum
