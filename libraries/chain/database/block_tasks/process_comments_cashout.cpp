#include <scorum/chain/database/block_tasks/process_comments_cashout.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/reward_fund.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

#include <scorum/rewards_math/curve.hpp>
#include <scorum/rewards_math/formulas.hpp>

#include <boost/range/adaptor/reversed.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_comments_cashout::on_apply(block_task_context& ctx)
{
    data_service_factory_i& services = ctx.services();
    reward_fund_service_i& reward_fund_service = services.reward_fund_service();
    comment_service_i& comment_service = services.comment_service();
    dynamic_global_property_service_i& dgp_service = services.dynamic_global_property_service();

    fc::time_point_sec head_block_time = dgp_service.head_block_time();

    auto comments = comment_service.get_by_cashout_time(head_block_time);

    const auto& rf = reward_fund_service.get();

    shares_vector_type total_rshares = get_total_rshares(ctx, comments);
    fc::uint128_t total_claims
        = rewards_math::calculate_total_claims(rf.recent_claims, head_block_time, rf.last_update,
                                               rf.author_reward_curve, total_rshares, SCORUM_RECENT_RSHARES_DECAY_RATE);

    std::map<account_name_type, asset> commenting_rewards;
    asset scorum_awarded = asset(0, SCORUM_SYMBOL);

    // newest, with bigger depth comments first
    for (const comment_object& comment : boost::adaptors::reverse(comments))
    {
        if (comment.net_rshares > 0)
        {
            auto payout = rewards_math::calculate_payout(
                comment.net_rshares, total_claims, rf.activity_reward_balance_scr.amount, rf.author_reward_curve,
                comment.max_accepted_payout.amount, SCORUM_MIN_COMMENT_PAYOUT_SHARE);

            asset commenting_reward = commenting_rewards[comment.author];

            auto payout_result = pay_for_comment(ctx, comment, asset(payout, SCORUM_SYMBOL), commenting_reward);
            scorum_awarded += payout_result.total_claimed_reward;

            commenting_rewards[comment.parent_author] += payout_result.parent_author_reward;
        }

        comment_service.update(comment, [&](comment_object& c) {
            /**
             * A payout is only made for positive rshares, negative rshares hang
             * around
             * for the next time this post might get an upvote.
             */
            if (c.net_rshares > 0)
            {
                c.net_rshares = 0;
            }
            c.children_abs_rshares = 0;
            c.abs_rshares = 0;
            c.vote_rshares = 0;
            c.total_vote_weight = 0;
            c.max_cashout_time = fc::time_point_sec::maximum();
            c.cashout_time = fc::time_point_sec::maximum();
            c.last_payout = head_block_time;
        });

        ctx.push_virtual_operation(comment_payout_update_operation(comment.author, fc::to_string(comment.permlink)));

#ifdef CLEAR_VOTES
        comment_vote_service_i& comment_vote_service = services.comment_vote_service();
        auto comment_votes = comment_vote_service.get_by_comment(comment.id);
        for (const comment_vote_object& vote : comment_votes)
        {
            comment_vote_service.remove(vote);
        }
#endif
    }

    // Write the cached fund state back to the database
    reward_fund_service.update([&](reward_fund_object& rfo) {
        rfo.recent_claims = total_claims;
        rfo.activity_reward_balance_scr -= scorum_awarded;
        rfo.last_update = head_block_time;
    });
}

shares_vector_type process_comments_cashout::get_total_rshares(block_task_context& ctx,
                                                               const comment_service_i::comment_refs_type& comments)
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

comment_payout_result process_comments_cashout::pay_for_comment(block_task_context& ctx,
                                                                const comment_object& comment,
                                                                const asset& reward,
                                                                const asset& commenting_reward)
{
    data_service_factory_i& services = ctx.services();
    account_service_i& account_service = services.account_service();
    comment_service_i& comment_service = services.comment_service();

    try
    {
        comment_payout_result payout_result;

        if (reward.amount > 0)
        {
            // clang-format off
            asset curation_tokens = asset(rewards_math::calculate_curations_payout(reward.amount, SCORUM_CURATION_REWARD_PERCENT), reward.symbol());
            asset author_tokens = reward - curation_tokens;

            author_tokens += pay_curators(ctx, comment, curation_tokens); // curation_tokens can be changed inside pay_curators()

            auto total_author_reward = author_tokens + commenting_reward;
            auto parent_author_reward = comment.depth == 0
                ? asset(0, SCORUM_SYMBOL)
                : total_author_reward * SCORUM_PARENT_COMMENT_REWARD_PERCENT / SCORUM_100_PERCENT;
            author_tokens = total_author_reward - parent_author_reward;

            payout_result.total_claimed_reward = author_tokens + curation_tokens;
            payout_result.parent_author_reward = parent_author_reward;

            auto total_beneficiary = asset(0, SCORUM_SYMBOL);
            for (auto& b : comment.beneficiaries)
            {
                asset benefactor_tokens = (author_tokens * b.weight) / SCORUM_100_PERCENT;
                asset sp_created = account_service.create_scorumpower(account_service.get_account(b.account), benefactor_tokens);
                ctx.push_virtual_operation(comment_benefactor_reward_operation(b.account, comment.author, fc::to_string(comment.permlink), sp_created));
                total_beneficiary += benefactor_tokens;
            }

            author_tokens -= total_beneficiary;

            comment_service.update(comment, [&](comment_object& c) {
                c.total_payout_value += author_tokens;
                c.curator_payout_value += curation_tokens;
                c.beneficiary_payout_value += total_beneficiary;
                c.parent_author_payout_value += payout_result.parent_author_reward;
            });

            auto payout_scorum = (author_tokens * comment.percent_scrs) / (2 * SCORUM_100_PERCENT);
            auto vesting_scorum = (author_tokens - payout_scorum);

            const auto& author = account_service.get_account(comment.author);
            account_service.increase_balance(author, payout_scorum);
            asset sp_created = account_service.create_scorumpower(author, vesting_scorum);

            ctx.push_virtual_operation(author_reward_operation(comment.author, fc::to_string(comment.permlink), payout_scorum, sp_created));
            ctx.push_virtual_operation(comment_reward_operation(comment.author, fc::to_string(comment.permlink), payout_result.total_claimed_reward));

#ifndef IS_LOW_MEM
            comment_service.update(comment, [&](comment_object& c) { c.author_rewards += author_tokens; });

            account_service.increase_posting_rewards(author, author_tokens);
#endif
        }

        return payout_result;
    }
    FC_CAPTURE_AND_RETHROW((comment))
}

asset
process_comments_cashout::pay_curators(block_task_context& ctx, const comment_object& comment, asset& max_rewards)
{
    data_service_factory_i& services = ctx.services();
    account_service_i& account_service = services.account_service();
    comment_vote_service_i& comment_vote_service = services.comment_vote_service();

    try
    {
        asset unclaimed_rewards = max_rewards;

        if (!comment.allow_curation_rewards)
        {
            // TODO: if allow_curation_rewards == false we loose money, it brings us to break invariants - need to
            // rework
            unclaimed_rewards = asset(0, SCORUM_SYMBOL);
            max_rewards = asset(0, SCORUM_SYMBOL);
        }
        else if (comment.total_vote_weight > 0)
        {
            auto comment_votes = comment_vote_service.get_by_comment_weight_voter(comment.id);
            for (const comment_vote_object& vote : comment_votes)
            {
                auto claim = asset(rewards_math::calculate_curation_payout(max_rewards.amount, comment.total_vote_weight, vote.weight) , max_rewards.symbol());
                if (claim.amount > 0)
                {
                    unclaimed_rewards -= claim;
                    const auto& voter = account_service.get(vote.voter);
                    auto reward = account_service.create_scorumpower(voter, claim);

                    ctx.push_virtual_operation(
                        curation_reward_operation(voter.name, reward, comment.author, fc::to_string(comment.permlink)));

#ifndef IS_LOW_MEM
                    account_service.increase_curation_rewards(voter, claim);
#endif
                }
            }
        }
        max_rewards -= unclaimed_rewards;

        return unclaimed_rewards;
    }
    FC_CAPTURE_AND_RETHROW()
}

}
}
}


