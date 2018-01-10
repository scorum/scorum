#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluator.hpp>
#include <scorum/chain/dbservice.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;

// clang-format off
 template <typename AccountService,
          typename ProposalService,
          typename CommitteeService,
          typename OperationType = scorum::protocol::operation>
 class proposal_create_evaluator_t : public evaluator<OperationType>
// clang-format on
{
public:
    //    typedef scorum::protocol::operation ;
    typedef proposal_create_evaluator_t<AccountService, ProposalService, CommitteeService> EvaluatorType;
    typedef proposal_create_operation operation_type;

    proposal_create_evaluator_t(AccountService& account_service,
                                ProposalService& proposal_service,
                                CommitteeService& committee_service,
                                uint32_t lifetime_min,
                                uint32_t lifetime_max,
                                uint64_t quorum_percent)
        : _account_service(account_service)
        , _proposal_service(proposal_service)
        , _committee_service(committee_service)
        , _lifetime_min(lifetime_min)
        , _lifetime_max(lifetime_max)
        , _quorum_percent(quorum_percent)
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
        _account_service.check_account_existence(op.committee_member);

        fc::time_point_sec expiration = _proposal_service.head_block_time() + op.lifetime_sec;

        _proposal_service.create(op.creator, op.committee_member, *op.action, expiration, _quorum_percent);
    }

protected:
    AccountService& _account_service;
    ProposalService& _proposal_service;
    CommitteeService& _committee_service;

    const uint32_t _lifetime_min;
    const uint32_t _lifetime_max;
    const uint64_t _quorum_percent;
};

typedef proposal_create_evaluator_t<dbs_account, dbs_proposal, dbs_registration_committee> proposal_create_evaluator;

} // namespace chain
} // namespace scorum
