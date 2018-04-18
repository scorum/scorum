#include <scorum/chain/database/block_tasks/process_comments_cashout.hpp>

#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/reward_objects.hpp>

#include <scorum/rewards_math/curve.hpp>
#include <scorum/rewards_math/formulas.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

class process_comments_cashout_impl
{
public:
    explicit process_comments_cashout_impl(block_task_context& ctx)
        : _ctx(ctx)
        , dgp_service(ctx.services().dynamic_global_property_service())
        , account_service(ctx.services().account_service())
        , comment_service(ctx.services().comment_service())
        , comment_vote_service(ctx.services().comment_vote_service())
    {
    }

    template <typename FundService> void apply(FundService& fund_service)
    {
        using fund_object_type = typename FundService::object_type;

        auto comments = comment_service.get_by_cashout_time();

        const auto& rf = fund_service.get();

        share_types total_rshares = get_total_rshares(comments);
        fc::uint128_t total_claims
            = rewards::calculate_total_claims(rf.recent_claims, dgp_service.head_block_time(), rf.last_update,
                                              rf.author_reward_curve, total_rshares, SCORUM_RECENT_RSHARES_DECAY_RATE);

        auto reward_symbol = rf.activity_reward_balance.symbol();
        asset reward = asset(0, reward_symbol);
        for (const comment_object& comment : comments)
        {
            if (comment.cashout_time > dgp_service.head_block_time())
                break;

            if (comment.net_rshares > 0)
            {
                auto payout = rewards::calculate_payout(
                    comment.net_rshares, total_claims, rf.activity_reward_balance.amount, rf.author_reward_curve,
                    comment.max_accepted_payout.amount, SCORUM_MIN_COMMENT_PAYOUT_SHARE);
                reward += pay_for_comment(comment, asset(payout, reward_symbol));
            }

            close_comment_payout(comment);
        }

        // Write the cached fund state back to the database
        fund_service.update([&](fund_object_type& rfo) {
            rfo.recent_claims = total_claims;
            rfo.activity_reward_balance -= reward;
            rfo.last_update = dgp_service.head_block_time();
        });
    }

private:
    share_types get_total_rshares(const comment_service_i::comment_refs_type& comments)
    {
        share_types ret;

        for (const comment_object& comment : comments)
        {
            if (comment.cashout_time > dgp_service.head_block_time())
                break;

            if (comment.net_rshares > 0)
            {
                ret.push_back(comment.net_rshares);
            }
        }

        return ret;
    }

    asset pay_for_comment(const comment_object& comment, const asset& reward)
    {
        try
        {
            auto reward_symbol = reward.symbol();
            if (reward.amount > 0)
            {
                // clang-format off
                asset curation_tokens = asset(rewards::calculate_curations_payout(reward.amount, SCORUM_CURATION_REWARD_PERCENT), reward_symbol);
                asset author_tokens = reward - curation_tokens;

                author_tokens += pay_curators(comment, curation_tokens); // curation_tokens can be changed inside pay_curators()

                asset claimed_reward = author_tokens + curation_tokens;

                auto total_beneficiary = asset(0, reward_symbol);
                for (auto& b : comment.beneficiaries)
                {
                    asset benefactor_tokens = (author_tokens * b.weight) / SCORUM_100_PERCENT;
                    pay_to(account_service.get_account(b.account), benefactor_tokens);
                    _ctx.push_virtual_operation(comment_benefactor_reward_operation(b.account, comment.author, fc::to_string(comment.permlink), benefactor_tokens));
                    total_beneficiary += benefactor_tokens;
                }

                author_tokens -= total_beneficiary;

                const auto& author = account_service.get_account(comment.author);
                pay_to(author, author_tokens);

                _ctx.push_virtual_operation(author_reward_operation(comment.author, fc::to_string(comment.permlink), author_tokens));
                _ctx.push_virtual_operation(comment_reward_operation(comment.author, fc::to_string(comment.permlink), claimed_reward));

                accumulate_statistic(comment, author, author_tokens, curation_tokens, total_beneficiary);

                return claimed_reward;
            }

            return asset(0, reward_symbol);
        }
        FC_CAPTURE_AND_RETHROW((comment))
    }

    asset
    pay_curators(const comment_object& comment, asset& max_rewards)
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
                    auto claim = asset(rewards::calculate_curation_payout(max_rewards.amount, comment.total_vote_weight, vote.weight) , reward_symbol);
                    if (claim.amount > 0)
                    {
                        unclaimed_rewards -= claim;

                        const auto& voter = account_service.get(vote.voter);
                        pay_to(voter, claim);

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

    void close_comment_payout(const comment_object& comment)
    {
        comment_service.update(comment, [&](comment_object& c) {
            /**
             * A payout is only made for positive rshares, negative rshares hang around
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

    void pay_to(const account_object &recipient, const asset &reward)
    {
        if (SCORUM_SYMBOL == reward.symbol())
        {
            account_service.increase_balance(recipient, reward);
        }else if (SP_SYMBOL == reward.symbol())
        {
            account_service.increase_scorumpower(recipient, reward);

            dgp_service.update(
                [&](dynamic_global_property_object& props) { props.total_scorumpower += reward; });
        }
    }

    void accumulate_statistic(const comment_object& comment, const account_object &author,
                              const asset &author_tokens, const asset &curation_tokens, const asset &total_beneficiary)
    {
        comment_service.update(comment, [&](comment_object& c) {
            c.total_payout_value += asset(author_tokens.amount, SCORUM_SYMBOL);
            c.curator_payout_value += asset(curation_tokens.amount, SCORUM_SYMBOL);
            c.beneficiary_payout_value += asset(total_beneficiary.amount, SCORUM_SYMBOL);
        });

#ifndef IS_LOW_MEM
        comment_service.update(comment, [&](comment_object& c) {
            c.author_rewards += asset(author_tokens.amount, SCORUM_SYMBOL);
        });

        account_service.increase_posting_rewards(author, asset(author_tokens.amount, SCORUM_SYMBOL));
#endif
    }

    void accumulate_statistic(const account_object &voter, const asset &claim)
    {
#ifndef IS_LOW_MEM
        account_service.increase_curation_rewards(voter, asset(claim.amount, SCORUM_SYMBOL));
#endif
    }

private:

    block_task_context& _ctx;
    dynamic_global_property_service_i& dgp_service;
    account_service_i& account_service;
    comment_service_i& comment_service;
    comment_vote_service_i& comment_vote_service;
};

void process_comments_cashout::on_apply(block_task_context& ctx)
{
    process_comments_cashout_impl impl(ctx);

    apply_for_scr_fund(ctx, impl);
    apply_for_sp_fund(ctx, impl);
}

void process_comments_cashout::apply_for_scr_fund(block_task_context& ctx, process_comments_cashout_impl &impl)
{
    reward_fund_scr_service_i& reward_fund_service = ctx.services().reward_fund_scr_service();

    if (reward_fund_service.get().activity_reward_balance.amount > 0)
    {
        impl.apply(reward_fund_service);
    }
}
void process_comments_cashout::apply_for_sp_fund(block_task_context& ctx, process_comments_cashout_impl &impl)
{
    reward_fund_sp_service_i& reward_fund_service = ctx.services().reward_fund_sp_service();

    if (reward_fund_service.get().activity_reward_balance.amount > 0)
    {
        impl.apply(reward_fund_service);
    }
}

}
}
}


