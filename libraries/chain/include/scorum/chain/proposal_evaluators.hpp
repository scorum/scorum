#pragma once

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>
#include <scorum/chain/evaluators/evaluator_registry.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/protocol/types.hpp>

#include <scorum/chain/schema/proposal_object.hpp>

#include <scorum/chain/services/registration_committee.hpp>

#include <scorum/chain/evaluators/committee_accessor.hpp>

namespace scorum {
namespace chain {

template <typename T>
using proposal_operation_evaluator
    = scorum::chain::evaluator_impl<data_service_factory_i, T, protocol::proposal_operation>;

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
        committee_i& committee_service = committee_accessor(this->db()).get_committee(o);
        committee_service.add_member(o.account_name);
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
        committee_i& committee_service = committee_accessor(this->db()).get_committee(o);
        committee_service.exclude_member(o.account_name);

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
        committee_i& committee_service = committee_accessor(this->db()).get_committee(o);

        if (o.committee_quorum == add_member_quorum)
        {
            committee_service.change_add_member_quorum(o.quorum);
        }
        else if (o.committee_quorum == exclude_member_quorum)
        {
            committee_service.change_exclude_member_quorum(o.quorum);
        }
        else if (o.committee_quorum == base_quorum)
        {
            committee_service.change_base_quorum(o.quorum);
        }
        else
        {
            FC_THROW_EXCEPTION(fc::assert_exception, "unknow quorum change operation");
        }
    }
};

namespace registration_committee {

using proposal_add_member_evaluator
    = scorum::chain::proposal_add_member_evaluator<registration_committee_add_member_operation>;

using proposal_exclude_member_evaluator
    = scorum::chain::proposal_exclude_member_evaluator<registration_committee_exclude_member_operation>;

using proposal_change_quorum_evaluator
    = scorum::chain::proposal_change_quorum_evaluator<registration_committee_change_quorum_operation>;

} // namespace registration_committee

} // namespace chain
} // namespace scorum
