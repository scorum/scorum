#include <scorum/chain/database/block_tasks/comments_cashout_impl.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

process_comments_cashout_impl::process_comments_cashout_impl(block_task_context& ctx)
    : _ctx(ctx)
    , dgp_service(ctx.services().dynamic_global_property_service())
    , account_service(ctx.services().account_service())
    , account_blogging_statistic_service(ctx.services().account_blogging_statistic_service())
    , comment_service(ctx.services().comment_service())
    , comment_statistic_scr_service(ctx.services().comment_statistic_scr_service())
    , comment_statistic_sp_service(ctx.services().comment_statistic_sp_service())
    , comment_vote_service(ctx.services().comment_vote_service())
{
}

void process_comments_cashout_impl::close_comment_payout(const comment_object& comment)
{
    comment_service.update(comment, [&](comment_object& c) {

        // TODO: remove abs_* fields (it is used only in tests)
        c.children_abs_rshares = 0;
        c.abs_rshares = 0;

        c.cashout_time = fc::time_point_sec::maximum();
        c.last_payout = dgp_service.head_block_time();
    });

    _ctx.push_virtual_operation(comment_payout_update_operation(comment.author, fc::to_string(comment.permlink)));

#ifdef CLEAR_VOTES
    auto comment_votes = comment_vote_service.get_by_comment(comment.id);
    for (const comment_vote_object& vote : comment_votes)
    {
        comment_vote_service.remove(vote);
    }
#endif
}

shares_vector_type
process_comments_cashout_impl::get_total_rshares(const comment_service_i::comment_refs_type& comments)
{
    shares_vector_type ret;

    for (const comment_object& comment : comments)
    {
        if (comment.net_rshares > 0)
        {
            ret.push_back(comment.net_rshares);
        }
    }

    return ret;
}

asset process_comments_cashout_impl::pay_for_comment(const comment_object& comment, const asset& reward)
{
    try
    {
        auto reward_symbol = reward.symbol();
        if (reward.amount < 1)
            return asset(0, reward_symbol);

        asset curation_tokens = asset(
            rewards_math::calculate_curations_payout(reward.amount, SCORUM_CURATION_REWARD_PERCENT), reward_symbol);
        asset author_tokens = reward - curation_tokens;

        author_tokens += pay_curators(comment, curation_tokens); // curation_tokens can be changed inside pay_curators()

        asset claimed_reward = author_tokens + curation_tokens;

        auto total_beneficiary = asset(0, reward_symbol);
        for (auto& b : comment.beneficiaries)
        {
            asset benefactor_tokens = (author_tokens * b.weight) / SCORUM_100_PERCENT;
            pay_account(account_service.get_account(b.account), benefactor_tokens);
            _ctx.push_virtual_operation(comment_benefactor_reward_operation(
                b.account, comment.author, fc::to_string(comment.permlink), benefactor_tokens));
            total_beneficiary += benefactor_tokens;
        }

        author_tokens -= total_beneficiary;

        const auto& author = account_service.get_account(comment.author);
        pay_account(author, author_tokens);

        _ctx.push_virtual_operation(
            author_reward_operation(comment.author, fc::to_string(comment.permlink), author_tokens));
        _ctx.push_virtual_operation(
            comment_reward_operation(comment.author, fc::to_string(comment.permlink), claimed_reward));

        accumulate_statistic(comment, author, author_tokens, curation_tokens, total_beneficiary, reward_symbol);

        return claimed_reward;
    }
    FC_CAPTURE_AND_RETHROW((comment))
}

asset process_comments_cashout_impl::pay_curators(const comment_object& comment, asset& max_rewards)
{
    try
    {
        auto reward_symbol = max_rewards.symbol();
        asset unclaimed_rewards = max_rewards;

        if (!comment.allow_curation_rewards)
        {
            // TODO: if allow_curation_rewards == false we loose money, it brings us to break invariants - need to
            // rework
            unclaimed_rewards = asset(0, reward_symbol);
            max_rewards = asset(0, reward_symbol);
        }
        else if (comment.total_vote_weight > 0)
        {
            auto comment_votes = comment_vote_service.get_by_comment_weight_voter(comment.id);
            for (const comment_vote_object& vote : comment_votes)
            {
                auto claim = asset(
                    rewards_math::calculate_curation_payout(max_rewards.amount, comment.total_vote_weight, vote.weight),
                    reward_symbol);
                if (claim.amount > 0)
                {
                    unclaimed_rewards -= claim;

                    const auto& voter = account_service.get(vote.voter);
                    pay_account(voter, claim);

                    _ctx.push_virtual_operation(
                        curation_reward_operation(voter.name, claim, comment.author, fc::to_string(comment.permlink)));

                    accumulate_statistic(voter, claim);
                }
            }
        }
        max_rewards -= unclaimed_rewards;

        return unclaimed_rewards;
    }
    FC_CAPTURE_AND_RETHROW()
}

void process_comments_cashout_impl::pay_account(const account_object& recipient, const asset& reward)
{
    if (SCORUM_SYMBOL == reward.symbol())
    {
        account_service.increase_balance(recipient, reward);
    }
    else if (SP_SYMBOL == reward.symbol())
    {
        account_service.create_scorumpower(recipient, reward);
    }
}

void process_comments_cashout_impl::accumulate_statistic(const comment_object& comment,
                                                         const account_object& author,
                                                         const asset& author_tokens,
                                                         const asset& curation_tokens,
                                                         const asset& total_beneficiary,
                                                         asset_symbol_type reward_symbol)
{
    FC_ASSERT(author_tokens.symbol() == reward_symbol);
    FC_ASSERT(curation_tokens.symbol() == reward_symbol);
    FC_ASSERT(total_beneficiary.symbol() == reward_symbol);

    asset total_payout = author_tokens;
    total_payout += curation_tokens;
    total_payout += total_beneficiary;

    if (SCORUM_SYMBOL == reward_symbol)
    {
        accumulate_comment_statistic(comment_statistic_scr_service, comment, total_payout, author_tokens,
                                     curation_tokens, total_beneficiary);
    }
    else if (SP_SYMBOL == reward_symbol)
    {
        accumulate_comment_statistic(comment_statistic_sp_service, comment, total_payout, author_tokens,
                                     curation_tokens, total_beneficiary);
    }

#ifndef IS_LOW_MEM
    {
        const auto& author_stat = account_blogging_statistic_service.obtain(author.id);
        account_blogging_statistic_service.increase_posting_rewards(author_stat, author_tokens);
    }
#endif
}

void process_comments_cashout_impl::accumulate_statistic(const account_object& voter, const asset& curation_tokens)
{
#ifndef IS_LOW_MEM
    const auto& voter_stat = account_blogging_statistic_service.obtain(voter.id);
    account_blogging_statistic_service.increase_curation_rewards(voter_stat, curation_tokens);
#endif
}
}
}
}
