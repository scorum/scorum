#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluator.hpp>
#include <scorum/chain/dbservice.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/data_service_factory.hpp>

namespace scorum {
namespace chain {

namespace sp = scorum::protocol;

template <typename OperationType = scorum::protocol::operation>
class proposal_create_evaluator_new : public evaluator<OperationType>
{
public:
    //    typedef sp::proposal_create_operation OperationType;
    typedef sp::proposal_create_operation operation_type;

    proposal_create_evaluator_new(scorum::chain::data_service_factory_i& services)
        : _services(services)
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

    void do_apply(const operation_type& op)
    {
        scorum::chain::account_service_i& account_service = _services.account_service();
        scorum::chain::proposal_service_i& proposal_service = _services.proposal_service();
        scorum::chain::committee_service_i& committee_service = _services.committee_service();
        //        scorum::chain::property_service_i& property_service = _services.property_service();

        FC_ASSERT((op.lifetime_sec <= SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS
                   && op.lifetime_sec >= SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS),
                  "Proposal life time is not in range of ${min} - ${max} seconds.",
                  ("min", SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS)("max", SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS));

        FC_ASSERT(committee_service.member_exists(op.creator), "Account \"${account_name}\" is not in committee.",
                  ("account_name", op.creator));

        account_service.check_account_existence(op.creator);
        account_service.check_account_existence(op.committee_member);

        fc::time_point_sec expiration = _services.head_block_time() + op.lifetime_sec;

        proposal_service.create(op.creator, op.committee_member, *op.action, expiration,
                                SCORUM_COMMITTEE_QUORUM_PERCENT);
    }

private:
    scorum::chain::data_service_factory_i& _services;
};

} // namespace chain
} // namespace scorum
