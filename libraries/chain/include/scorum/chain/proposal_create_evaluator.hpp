#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluator.hpp>
#include <scorum/chain/dbservice.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/dbs_registration_pool.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;

// clang-format off
 template <typename AccountService,
          typename ProposalService,
          typename CommitteeService,
          typename PoolService,
          typename OperationType = scorum::protocol::operation>
 class proposal_create_evaluator_t : public evaluator<OperationType>
// clang-format on
{
public:
    typedef proposal_create_operation operation_type;

    proposal_create_evaluator_t(AccountService& account_service,
                                ProposalService& proposal_service,
                                CommitteeService& committee_service,
                                PoolService& pool_service,
                                uint32_t lifetime_min,
                                uint32_t lifetime_max)
        : _account_service(account_service)
        , _proposal_service(proposal_service)
        , _committee_service(committee_service)
        , _pool_service(pool_service)
        , _lifetime_min(lifetime_min)
        , _lifetime_max(lifetime_max)
    {
    }

    virtual void apply(const OperationType& o) final override
    {
        const auto& op = o.template get<operation_type>();

        this->do_apply(op);
    }

    virtual int get_type() const override
    {
        return OperationType::template tag<operation_type>::value;
    }

    void do_apply(const proposal_create_operation& op)
    {
        FC_ASSERT((op.lifetime_sec <= _lifetime_max && op.lifetime_sec >= _lifetime_min),
                  "Proposal life time is not in range of ${min} - ${max} seconds.",
                  ("min", _lifetime_min)("max", _lifetime_max));

        FC_ASSERT(_committee_service.member_exists(op.creator), "Account \"${account_name}\" is not in committee.",
                  ("account_name", op.creator));

        _account_service.check_account_existence(op.creator);

        fc::time_point_sec expiration = _proposal_service.head_block_time() + op.lifetime_sec;

        _proposal_service.create(op.creator, op.data, *op.action, expiration, get_quorum(*op.action));
    }

    uint64_t get_quorum(proposal_action action)
    {
        const registration_pool_object& pool = _pool_service.get_pool();

        switch (action)
        {
        case proposal_action::invite:
            return pool.invite_quorum;

        case proposal_action::dropout:
            return pool.dropout_quorum;

        case proposal_action::change_invite_quorum:
        case proposal_action::change_dropout_quorum:
        case proposal_action::change_quorum:
            return pool.change_quorum;

        default:
            FC_ASSERT("invalid action type.");
        }

        return SCORUM_COMMITTEE_QUORUM_PERCENT;
    }

protected:
    AccountService& _account_service;
    ProposalService& _proposal_service;
    CommitteeService& _committee_service;
    PoolService& _pool_service;

    const uint32_t _lifetime_min;
    const uint32_t _lifetime_max;
};

typedef proposal_create_evaluator_t<dbs_account, dbs_proposal, dbs_registration_committee, dbs_registration_pool>
    proposal_create_evaluator;

} // namespace chain
} // namespace scorum
