#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluator.hpp>
#include <scorum/chain/dbservice.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_proposal.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;

// clang-format off
 template <typename AccountService,
          typename ProposalService,
          typename OperationType = scorum::protocol::operation>
 class proposal_create_evaluator_t : public evaluator<OperationType>
// clang-format on
{
public:
    //    typedef scorum::protocol::operation ;
    typedef proposal_create_evaluator_t<AccountService, ProposalService> EvaluatorType;
    typedef proposal_create_operation operation_type;

    proposal_create_evaluator_t(AccountService& account_service,
                                ProposalService& proposal_service,
                                uint32_t lifetime_min,
                                uint32_t lifetime_max)
        : _account_service(account_service)
        , _proposal_service(proposal_service)
        , _lifetime_min(lifetime_min)
        , _lifetime_max(lifetime_max)
    {
    }

    virtual void apply(const OperationType& o) final override
    {
        auto* eval = static_cast<EvaluatorType*>(this);
        const auto& op = o.template get<typename EvaluatorType::operation_type>();
        eval->do_apply(op);
    }

    virtual int get_type() const override
    {
        return OperationType::template tag<typename EvaluatorType::operation_type>::value;
    }

    void do_apply(const proposal_create_operation& op)
    {
        FC_ASSERT((op.lifetime_sec <= _lifetime_max && op.lifetime_sec >= _lifetime_min),
                  "Proposal life time is not in range of ${min} - ${max} seconds.",
                  ("min", _lifetime_min)("max", _lifetime_max));

        FC_ASSERT(_account_service.is_exists(op.creator), "Account \"${account_name}\" must exist.",
                  ("account_name", op.creator));

        FC_ASSERT(_account_service.is_exists(op.committee_member), "Account \"${account_name}\" must exist.",
                  ("account_name", op.committee_member));

        FC_ASSERT(op.action.valid(), "Proposal is not set.");

        fc::time_point_sec expiration = _proposal_service.head_block_time() + op.lifetime_sec;

        _proposal_service.create(op.creator, op.committee_member, *op.action, expiration);
    }

protected:
    AccountService& _account_service;
    ProposalService& _proposal_service;

    const uint32_t _lifetime_min;
    const uint32_t _lifetime_max;
};

typedef proposal_create_evaluator_t<dbs_account, dbs_proposal> proposal_create_evaluator;

} // namespace chain
} // namespace scorum
