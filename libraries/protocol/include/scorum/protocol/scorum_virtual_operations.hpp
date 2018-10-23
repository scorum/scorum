#pragma once
#include <scorum/protocol/base.hpp>
#include <scorum/protocol/block_header.hpp>
#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/betting/game_status.hpp>
#include <scorum/protocol/types.hpp>

#include <fc/utf8.hpp>

namespace scorum {
namespace protocol {

struct author_reward_operation : public virtual_operation
{
    author_reward_operation()
    {
    }
    author_reward_operation(const account_name_type& a, const std::string& p, const asset& reward_)
        : author(a)
        , permlink(p)
        , reward(reward_)
    {
    }

    account_name_type author;
    std::string permlink;

    /// all reward for comment author (with from_children_payout)
    asset reward; // in SCR or SP
};

struct curation_reward_operation : public virtual_operation
{
    curation_reward_operation()
    {
    }
    curation_reward_operation(const std::string& c, const asset& r, const std::string& a, const std::string& p)
        : curator(c)
        , reward(r)
        , comment_author(a)
        , comment_permlink(p)
    {
    }

    account_name_type curator;
    asset reward; // in SCR or SP
    account_name_type comment_author;
    std::string comment_permlink;
};

struct comment_reward_operation : public virtual_operation
{
    comment_reward_operation()
    {
    }
    comment_reward_operation(const account_name_type& a,
                             const std::string& pl,
                             const asset& fund_reward,
                             const asset& total_payout,
                             const asset& author_payout,
                             const asset& curators_payout,
                             const asset& from_children_payout,
                             const asset& to_parent_payout,
                             const asset& beneficiaries_payout)
        : author(a)
        , permlink(pl)
        , fund_reward(fund_reward)
        , total_payout(total_payout)
        , author_payout(author_payout)
        , curators_payout(curators_payout)
        , from_children_payout(from_children_payout)
        , to_parent_payout(to_parent_payout)
        , beneficiaries_payout(beneficiaries_payout)
    {
    }

    account_name_type author;
    std::string permlink;

    /// reward accrued from fund
    asset fund_reward;

    /// reward distributed within comment across author, beneficiaries and curators (excluding reward to parent)
    asset total_payout;

    /// reward for comment author (including from_children_payout)
    asset author_payout;

    /// reward for curators (voters)
    asset curators_payout;

    /// reward received from children comments as parent reward
    asset from_children_payout;

    /// reward sent to parent  as parent reward
    asset to_parent_payout;

    /// reward for beneficiaries
    asset beneficiaries_payout;
};

struct acc_finished_vesting_withdraw_operation : public virtual_operation
{
    acc_finished_vesting_withdraw_operation() = default;
    acc_finished_vesting_withdraw_operation(const std::string& f)
        : from_account(f)
    {
    }

    account_name_type from_account;
};

struct devpool_finished_vesting_withdraw_operation : public virtual_operation
{
    devpool_finished_vesting_withdraw_operation() = default;
};

struct acc_to_acc_vesting_withdraw_operation : public virtual_operation
{
    acc_to_acc_vesting_withdraw_operation() = default;
    acc_to_acc_vesting_withdraw_operation(const std::string& f, const std::string& t, const asset& w)
        : from_account(f)
        , to_account(t)
        , withdrawn(w)
    {
    }

    account_name_type from_account;
    account_name_type to_account;
    asset withdrawn = asset(0, SP_SYMBOL);
};

struct devpool_to_acc_vesting_withdraw_operation : public virtual_operation
{
    devpool_to_acc_vesting_withdraw_operation() = default;
    devpool_to_acc_vesting_withdraw_operation(const std::string& t, const asset& w)
        : to_account(t)
        , withdrawn(w)
    {
    }

    account_name_type to_account;
    asset withdrawn = asset(0, SP_SYMBOL);
};

struct acc_to_devpool_vesting_withdraw_operation : public virtual_operation
{
    acc_to_devpool_vesting_withdraw_operation() = default;
    acc_to_devpool_vesting_withdraw_operation(const std::string& f, const asset& w)
        : from_account(f)
        , withdrawn(w)
    {
    }

    account_name_type from_account;
    asset withdrawn = asset(0, SP_SYMBOL);
};

struct devpool_to_devpool_vesting_withdraw_operation : public virtual_operation
{
    devpool_to_devpool_vesting_withdraw_operation() = default;
    explicit devpool_to_devpool_vesting_withdraw_operation(const asset& w)
        : withdrawn(w)
    {
    }

    asset withdrawn = asset(0, SP_SYMBOL);
};

struct shutdown_witness_operation : public virtual_operation
{
    shutdown_witness_operation()
    {
    }
    shutdown_witness_operation(const std::string& o)
        : owner(o)
    {
    }

    account_name_type owner;
};

struct witness_miss_block_operation : public virtual_operation
{
    witness_miss_block_operation()
    {
    }
    witness_miss_block_operation(const std::string& o, uint32_t num)
        : owner(o)
        , block_num(num)
    {
    }

    account_name_type owner;
    uint32_t block_num = 0;
};

struct hardfork_operation : public virtual_operation
{
    hardfork_operation()
    {
    }
    hardfork_operation(uint32_t hf_id)
        : hardfork_id(hf_id)
    {
    }

    uint32_t hardfork_id = 0;
};

struct comment_payout_update_operation : public virtual_operation
{
    comment_payout_update_operation()
    {
    }
    comment_payout_update_operation(const account_name_type& a, const std::string& p)
        : author(a)
        , permlink(p)
    {
    }

    account_name_type author;
    std::string permlink;
};

struct return_scorumpower_delegation_operation : public virtual_operation
{
    return_scorumpower_delegation_operation()
    {
    }
    return_scorumpower_delegation_operation(const account_name_type& a, const asset& v)
        : account(a)
        , scorumpower(v)
    {
    }

    account_name_type account;
    asset scorumpower = asset(0, SP_SYMBOL);
};

struct comment_benefficiary_reward_operation : public virtual_operation
{
    comment_benefficiary_reward_operation()
    {
    }
    comment_benefficiary_reward_operation(const account_name_type& b,
                                          const account_name_type& a,
                                          const std::string& p,
                                          const asset& r)
        : benefactor(b)
        , author(a)
        , permlink(p)
        , reward(r)
    {
    }

    account_name_type benefactor;
    account_name_type author;
    std::string permlink;
    asset reward; // in SCR or SP
};

struct producer_reward_operation : public virtual_operation
{
    producer_reward_operation()
    {
    }
    producer_reward_operation(const std::string& p, const asset& v)
        : producer(p)
        , reward(v)
    {
    }

    account_name_type producer;
    asset reward; // in SCR or SP
};

struct active_sp_holders_reward_operation : public virtual_operation
{
    active_sp_holders_reward_operation()
    {
    }
    active_sp_holders_reward_operation(const std::string& h, const asset& v)
        : sp_holder(h)
        , reward(v)
    {
    }

    account_name_type sp_holder;
    asset reward; // in SCR or SP
};

struct active_sp_holders_reward_legacy_operation : public virtual_operation
{
    active_sp_holders_reward_legacy_operation() = default;

    using rewarded_type = fc::flat_map<account_name_type, asset>;

    active_sp_holders_reward_legacy_operation(rewarded_type&& rewarded_)
        : rewarded(rewarded_)
    {
    }

    /// rewards map in SCR or SP
    rewarded_type rewarded;
};

struct expired_contract_refund_operation : public virtual_operation
{
    expired_contract_refund_operation()
    {
    }
    expired_contract_refund_operation(const std::string& o, const asset& v)
        : owner(o)
        , refund(v)
    {
    }

    account_name_type owner;
    asset refund = asset(0, SCORUM_SYMBOL);
};

struct proposal_virtual_operation : public virtual_operation
{
    proposal_virtual_operation() = default;
    proposal_virtual_operation(const protocol::proposal_operation& op)
        : proposal_op(op)
    {
    }

    protocol::proposal_operation proposal_op;
};

struct budget_outgo_operation : public virtual_operation
{
    budget_outgo_operation()
    {
    }
    budget_outgo_operation(const budget_type type_,
                           const account_name_type& owner_,
                           const int64_t id_,
                           const asset& cash_)
        : type(type_)
        , owner(owner_)
        , id(id_)
        , cash(cash_)
    {
        FC_ASSERT(cash.symbol() == SCORUM_SYMBOL);
    }

    budget_type type = budget_type::post;
    account_name_type owner;
    int64_t id = -1;
    asset cash = asset(0, SCORUM_SYMBOL);
};

struct budget_owner_income_operation : public virtual_operation
{
    budget_owner_income_operation()
    {
    }
    budget_owner_income_operation(const budget_type type_,
                                  const account_name_type& owner_,
                                  const int64_t id_,
                                  const asset& cash_)
        : type(type_)
        , owner(owner_)
        , id(id_)
        , cash(cash_)
    {
        FC_ASSERT(cash.symbol() == SCORUM_SYMBOL);
    }

    budget_type type = budget_type::post;
    account_name_type owner;
    int64_t id = -1;
    asset cash = asset(0, SCORUM_SYMBOL);
};

struct budget_closing_operation : public virtual_operation
{
    budget_closing_operation()
    {
    }
    budget_closing_operation(const budget_type type, const account_name_type& owner, const int64_t id)
        : type(type)
        , owner(owner)
        , id(id)
    {
    }

    budget_type type = budget_type::post;
    account_name_type owner;
    int64_t id = -1;
};

struct bets_matched_operation : public virtual_operation
{
    bets_matched_operation() = default;
    bets_matched_operation(account_name_type better1,
                           account_name_type better2,
                           uuid_type bet1_uuid,
                           uuid_type bet2_uuid,
                           asset matched_stake1,
                           asset matched_stake2,
                           int64_t matched_bet_id)
        : better1(better1)
        , better2(better2)
        , bet1_uuid(bet1_uuid)
        , bet2_uuid(bet2_uuid)
        , matched_stake1(matched_stake1)
        , matched_stake2(matched_stake2)
        , matched_bet_id(matched_bet_id)
    {
    }

    account_name_type better1;
    account_name_type better2;
    uuid_type bet1_uuid;
    uuid_type bet2_uuid;
    asset matched_stake1 = asset(0, SCORUM_SYMBOL);
    asset matched_stake2 = asset(0, SCORUM_SYMBOL);
    int64_t matched_bet_id = -1;
};

struct game_status_changed_operation : public virtual_operation
{
    // TODO: could be removed in C++17
    game_status_changed_operation() = default;
    game_status_changed_operation(const uuid_type& game_uuid, game_status old_status, game_status new_status)
        : game_uuid(game_uuid)
        , old_status(old_status)
        , new_status(new_status)
    {
    }

    uuid_type game_uuid;
    game_status old_status;
    game_status new_status;
};

enum class bet_resolve_kind : uint8_t
{
    win,
    draw
};

struct bet_resolved_operation : public virtual_operation
{
    // TODO: could be removed in C++17
    bet_resolved_operation() = default;
    bet_resolved_operation(const uuid_type& game_uuid,
                           const account_name_type& better,
                           const uuid_type& bet_uuid,
                           const asset& income,
                           bet_resolve_kind kind)
        : game_uuid(game_uuid)
        , better(better)
        , bet_uuid(bet_uuid)
        , income(income)
        , kind(kind)
    {
    }

    uuid_type game_uuid;
    account_name_type better;
    uuid_type bet_uuid;
    asset income;
    bet_resolve_kind kind;
};

enum class bet_kind : uint8_t
{
    pending,
    matched
};

struct bet_cancelled_operation : public virtual_operation
{
    // TODO: could be removed in C++17
    bet_cancelled_operation() = default;
    bet_cancelled_operation(const uuid_type& game_uuid,
                            const account_name_type& better,
                            const uuid_type& bet_uuid,
                            const asset& stake,
                            bet_kind kind)
        : game_uuid(game_uuid)
        , better(better)
        , bet_uuid(bet_uuid)
        , stake(stake)
        , kind(kind)
    {
    }

    uuid_type game_uuid;
    account_name_type better;
    uuid_type bet_uuid;
    asset stake;
    bet_kind kind;
};
}
} // scorum::protocol

// clang-format off
FC_REFLECT(scorum::protocol::author_reward_operation,
           (author)
           (permlink)
           (reward))
FC_REFLECT(scorum::protocol::curation_reward_operation,
           (curator)
           (reward)
           (comment_author)
           (comment_permlink))
FC_REFLECT(scorum::protocol::comment_reward_operation,
           (author)
           (permlink)
           (fund_reward)
           (total_payout)
           (author_payout)
           (curators_payout)
           (from_children_payout)
           (to_parent_payout)
           (beneficiaries_payout))
FC_REFLECT(scorum::protocol::shutdown_witness_operation,
           (owner))
FC_REFLECT(scorum::protocol::witness_miss_block_operation,
           (owner)
           (block_num))
FC_REFLECT(scorum::protocol::hardfork_operation,
           (hardfork_id))
FC_REFLECT(scorum::protocol::comment_payout_update_operation,
           (author)
           (permlink))
FC_REFLECT(scorum::protocol::return_scorumpower_delegation_operation,
           (account)
           (scorumpower))
FC_REFLECT(scorum::protocol::comment_benefficiary_reward_operation,
           (benefactor)
           (author)
           (permlink)
           (reward))
FC_REFLECT(scorum::protocol::producer_reward_operation,
           (producer)
           (reward))
FC_REFLECT(scorum::protocol::active_sp_holders_reward_operation,
           (sp_holder)
           (reward))
FC_REFLECT(scorum::protocol::active_sp_holders_reward_legacy_operation,
           (rewarded))
FC_REFLECT(scorum::protocol::expired_contract_refund_operation,
           (owner)
           (refund))
FC_REFLECT(scorum::protocol::acc_finished_vesting_withdraw_operation,
           (from_account))
FC_REFLECT_EMPTY(scorum::protocol::devpool_finished_vesting_withdraw_operation)
FC_REFLECT(scorum::protocol::acc_to_acc_vesting_withdraw_operation,
           (from_account)
           (to_account)
           (withdrawn))
FC_REFLECT(scorum::protocol::devpool_to_acc_vesting_withdraw_operation,
           (to_account)
           (withdrawn))
FC_REFLECT(scorum::protocol::acc_to_devpool_vesting_withdraw_operation,
           (from_account)
           (withdrawn))
FC_REFLECT(scorum::protocol::devpool_to_devpool_vesting_withdraw_operation,
           (withdrawn))
FC_REFLECT(scorum::protocol::proposal_virtual_operation,
           (proposal_op))
FC_REFLECT(scorum::protocol::budget_outgo_operation,
           (type)
           (owner)
           (id)
           (cash))
FC_REFLECT(scorum::protocol::budget_owner_income_operation,
           (type)
           (owner)
           (id)
           (cash))
FC_REFLECT(scorum::protocol::budget_closing_operation,
           (type)
           (owner)
           (id))
FC_REFLECT(scorum::protocol::game_status_changed_operation,
           (game_uuid)
           (old_status)
           (new_status))
FC_REFLECT_ENUM(scorum::protocol::bet_resolve_kind,
           (win)
           (draw))
FC_REFLECT(scorum::protocol::bet_resolved_operation,
           (game_uuid)
           (better)
           (bet_uuid)
           (income)
           (kind))
FC_REFLECT_ENUM(scorum::protocol::bet_kind,
           (pending)
           (matched))
FC_REFLECT(scorum::protocol::bet_cancelled_operation,
           (game_uuid)
           (better)
           (bet_uuid)
           (stake)
           (kind))
FC_REFLECT(scorum::protocol::bets_matched_operation, (better1)(better2)(bet1_uuid)
           (bet2_uuid)
           (matched_stake1)
           (matched_stake2)
           (matched_bet_id))
// clang-format on
