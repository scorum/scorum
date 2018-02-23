#pragma once

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

template <typename OperationType>
struct proposal_evaluator_impl : public evaluator_impl<data_service_factory_i,
                                                       proposal_evaluator_impl<OperationType>,
                                                       protocol::proposal_operation>
{
    typedef OperationType operation_type;

    proposal_evaluator_impl(data_service_factory_i& factory)
        : evaluator_impl<data_service_factory_i, proposal_evaluator_impl<OperationType>, protocol::proposal_operation>(
              factory)
    {
    }

    virtual void do_apply(const OperationType&) = 0;
};

template <typename OperationType> struct proposal_add_member_evaluator : public proposal_evaluator_impl<OperationType>
{
    typedef OperationType operation_type;

    proposal_add_member_evaluator(data_service_factory_i& factory)
        : proposal_evaluator_impl<OperationType>(factory)
    {
    }

    void do_apply(const OperationType& o) override
    {
        //        const typename OperationType::base_operation_type& bo = o;
        //        auto& committee = this->db().obtain_committee(bo);

        //        committee.add_member(o.new_member);
    }
};

template <typename OperationType> struct set_quorum_evaluator : public proposal_evaluator_impl<OperationType>
{
    typedef OperationType operation_type;

    set_quorum_evaluator(data_service_factory_i& factory)
        : proposal_evaluator_impl<OperationType>(factory)
    {
    }

    void do_apply(const OperationType& o) override
    {
        //        const typename OperationType::base_operation_type& bo = o;
        //        auto& committee = this->db().obtain_committee(bo);

        //        committee.add_member(o.new_member);
    }
};

} // namespace chain
} // namespace scorum
