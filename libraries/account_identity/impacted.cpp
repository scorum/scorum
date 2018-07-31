#include <scorum/account_identity/impacted.hpp>

#include <scorum/protocol/authority.hpp>

#include <fc/utility.hpp>

namespace scorum {
namespace account_identity {

using namespace scorum::protocol;

struct get_impacted_account_visitor
{
    get_impacted_account_visitor(fc::flat_set<account_name_type>& impact)
        : _impacted(impact)
    {
    }
    typedef void result_type;

    template <typename T> void operator()(const T& op)
    {
        op.get_required_posting_authorities(_impacted);
        op.get_required_active_authorities(_impacted);
        op.get_required_owner_authorities(_impacted);
    }

    // ops
    void operator()(const account_create_operation& op)
    {
        _impacted.insert(op.new_account_name);
        _impacted.insert(op.creator);
    }

    void operator()(const account_create_with_delegation_operation& op)
    {
        _impacted.insert(op.new_account_name);
        _impacted.insert(op.creator);
    }

    void operator()(const account_create_by_committee_operation& op)
    {
        _impacted.insert(op.new_account_name);
        _impacted.insert(op.creator);
    }

    void operator()(const comment_operation& op)
    {
        _impacted.insert(op.author);
        if (op.parent_author.size())
            _impacted.insert(op.parent_author);
    }

    void operator()(const vote_operation& op)
    {
        _impacted.insert(op.voter);
        _impacted.insert(op.author);
    }

    void operator()(const transfer_operation& op)
    {
        _impacted.insert(op.from);
        _impacted.insert(op.to);
    }

    void operator()(const escrow_transfer_operation& op)
    {
        _impacted.insert(op.from);
        _impacted.insert(op.to);
        _impacted.insert(op.agent);
    }

    void operator()(const escrow_approve_operation& op)
    {
        _impacted.insert(op.from);
        _impacted.insert(op.to);
        _impacted.insert(op.agent);
    }

    void operator()(const escrow_dispute_operation& op)
    {
        _impacted.insert(op.from);
        _impacted.insert(op.to);
        _impacted.insert(op.agent);
    }

    void operator()(const escrow_release_operation& op)
    {
        _impacted.insert(op.from);
        _impacted.insert(op.to);
        _impacted.insert(op.agent);
    }

    void operator()(const transfer_to_scorumpower_operation& op)
    {
        _impacted.insert(op.from);

        if (op.to != account_name_type() && op.to != op.from)
        {
            _impacted.insert(op.to);
        }
    }

    void operator()(const set_withdraw_scorumpower_route_to_account_operation& op)
    {
        _impacted.insert(op.from_account);
        _impacted.insert(op.to_account);
    }

    void operator()(const set_withdraw_scorumpower_route_to_dev_pool_operation& op)
    {
        _impacted.insert(op.from_account);
    }

    void operator()(const account_witness_vote_operation& op)
    {
        _impacted.insert(op.account);
        _impacted.insert(op.witness);
    }

    void operator()(const account_witness_proxy_operation& op)
    {
        _impacted.insert(op.account);
        _impacted.insert(op.proxy);
    }

    void operator()(const request_account_recovery_operation& op)
    {
        _impacted.insert(op.account_to_recover);
        _impacted.insert(op.recovery_account);
    }

    void operator()(const recover_account_operation& op)
    {
        _impacted.insert(op.account_to_recover);
    }

    void operator()(const change_recovery_account_operation& op)
    {
        _impacted.insert(op.account_to_recover);
    }

    void operator()(const delegate_scorumpower_operation& op)
    {
        _impacted.insert(op.delegator);
        _impacted.insert(op.delegatee);
    }

    void operator()(const create_budget_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const close_budget_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const proposal_create_operation& op)
    {
        _impacted.insert(op.creator);
    }

    void operator()(const proposal_vote_operation& op)
    {
        _impacted.insert(op.voting_account);
    }

    void operator()(const atomicswap_initiate_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const atomicswap_redeem_operation& op)
    {
        _impacted.insert(op.to);
    }

    void operator()(const atomicswap_refund_operation& op)
    {
        _impacted.insert(op.participant);
    }

    // virtual operations

    void operator()(const author_reward_operation& op)
    {
        _impacted.insert(op.author);
    }

    void operator()(const curation_reward_operation& op)
    {
        _impacted.insert(op.curator);
    }

    void operator()(const comment_reward_operation& op)
    {
        _impacted.insert(op.author);
    }

    void operator()(const acc_to_acc_vesting_withdraw_operation& op)
    {
        _impacted.insert(op.from_account);
        _impacted.insert(op.to_account);
    }

    void operator()(const acc_to_devpool_vesting_withdraw_operation& op)
    {
        _impacted.insert(op.from_account);
    }

    void operator()(const devpool_to_acc_vesting_withdraw_operation& op)
    {
        _impacted.insert(op.to_account);
    }

    void operator()(const shutdown_witness_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const witness_miss_block_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const comment_payout_update_operation& op)
    {
        _impacted.insert(op.author);
    }

    void operator()(const active_sp_holders_reward_operation& op)
    {
        _impacted.insert(op.sp_holder);
    }

    void operator()(const return_scorumpower_delegation_operation& op)
    {
        _impacted.insert(op.account);
    }

    void operator()(const comment_benefactor_reward_operation& op)
    {
        _impacted.insert(op.benefactor);
        _impacted.insert(op.author);
    }

    void operator()(const producer_reward_operation& op)
    {
        _impacted.insert(op.producer);
    }

    void operator()(const expired_contract_refund_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const proposal_virtual_operation& op)
    {
        op.proposal_op.weak_visit(
            [&](const development_committee_transfer_operation& op) { _impacted.insert(op.to_account); });
    }

    void operator()(const acc_finished_vesting_withdraw_operation& op)
    {
        _impacted.insert(op.from_account);
    }

private:
    fc::flat_set<account_name_type>& _impacted;
};

void operation_get_impacted_accounts(const operation& op, fc::flat_set<account_name_type>& result)
{
    get_impacted_account_visitor v = get_impacted_account_visitor(result);
    op.visit(v);
}

void transaction_get_impacted_accounts(const transaction& tx, fc::flat_set<account_name_type>& result)
{
    for (const auto& op : tx.operations)
        operation_get_impacted_accounts(op, result);
}
}
}
