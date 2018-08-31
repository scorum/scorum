#pragma once
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/protocol/proposal_operations.hpp>
#include <actor.hpp>
#include "database_trx_integration.hpp"

namespace devcommittee_fixture {
using namespace scorum;
using namespace scorum::protocol;

struct devcommittee_fixture : public database_fixture::database_trx_integration_fixture

{
    template <typename... TVoter>
    void devcommittee_add_member(const Actor& devcommittee_member, const Actor& new_member, const TVoter&... voters)
    {
        development_committee_add_member_operation add_member_op;
        add_member_op.account_name = new_member.name;

        proposal_create_operation proposal_create_op;
        proposal_create_op.creator = devcommittee_member.name;
        proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
        proposal_create_op.operation = add_member_op;

        push_operation(proposal_create_op, devcommittee_member.private_key);

        vote(get_next_proposal_id(), voters...);
    }

    template <typename... TVoter>
    void devcommittee_change_quorum(const Actor& devcommittee_member,
                                    quorum_type quorum_type,
                                    protocol::percent_type quorum,
                                    const TVoter&... voters)
    {
        development_committee_change_quorum_operation change_quorum_op;
        change_quorum_op.committee_quorum = quorum_type;
        change_quorum_op.quorum = quorum;

        proposal_create_operation proposal_create_op;
        proposal_create_op.creator = devcommittee_member.name;
        proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
        proposal_create_op.operation = change_quorum_op;

        push_operation(proposal_create_op, devcommittee_member.private_key);

        vote(get_next_proposal_id(), voters...);
    }

    template <typename... TVoter>
    void
    devcommittee_exclude_member(const Actor& devcommittee_member, const Actor& excluded_member, const TVoter&... voters)
    {
        development_committee_exclude_member_operation exclude_member_op;
        exclude_member_op.account_name = excluded_member.name;

        proposal_create_operation proposal_create_op;
        proposal_create_op.creator = devcommittee_member.name;
        proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
        proposal_create_op.operation = exclude_member_op;

        push_operation(proposal_create_op, devcommittee_member.private_key);

        vote(get_next_proposal_id(), voters...);
    }

    template <typename... TVoter>
    void devcommittee_withdraw(const Actor& devcommittee_member, const asset& to_withdraw, const TVoter&... voters)
    {
        development_committee_withdraw_vesting_operation withdraw_op;
        withdraw_op.vesting_shares = to_withdraw;

        proposal_create_operation proposal_create_op;
        proposal_create_op.creator = devcommittee_member.name;
        proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
        proposal_create_op.operation = withdraw_op;

        push_operation(proposal_create_op, devcommittee_member.private_key);

        vote(get_next_proposal_id(), voters...);
    }

    template <typename... TVoter>
    void devcommittee_transfer(const Actor& devcommittee_member,
                               const Actor& target_acc,
                               const asset& amount,
                               const TVoter&... voters)
    {
        development_committee_transfer_operation transfer_op;
        transfer_op.to_account = target_acc.name;
        transfer_op.amount = amount;

        proposal_create_operation proposal_create_op;
        proposal_create_op.creator = devcommittee_member.name;
        proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
        proposal_create_op.operation = transfer_op;

        push_operation(proposal_create_op, devcommittee_member.private_key);

        vote(get_next_proposal_id(), voters...);
    }

    void wait_withdraw(uint32_t intervals)
    {
        for (uint32_t ci = 0; ci < intervals; ++ci)
        {
            auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
            generate_blocks(next_withdrawal, true);
        }
    }

private:
    template <typename... TVoters> void vote(int64_t proposal_id, const TVoters&... voters)
    {
        using expander = int[];
        (void)expander{ 0, (vote_impl(proposal_id, voters), 0)... };
    }

    void vote_impl(int64_t proposal_id, const Actor& voter)
    {
        proposal_vote_operation proposal_vote_op;
        proposal_vote_op.voting_account = voter.name;
        proposal_vote_op.proposal_id = proposal_id;

        push_operation(proposal_vote_op, voter.private_key);
    }

private:
    int64_t _proposal_id = -1;
    int64_t get_next_proposal_id()
    {
        return ++_proposal_id;
    }
};
}
