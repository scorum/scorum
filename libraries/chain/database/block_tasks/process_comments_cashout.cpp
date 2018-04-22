#include <scorum/chain/database/block_tasks/process_comments_cashout.hpp>

#include <scorum/chain/services/comment.hpp>
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

using scorum::rewards_math::shares_vector_type;
using comment_refs_type = scorum::chain::comment_service_i::comment_refs_type;

class process_comments_cashout_impl
{
public:
    explicit process_comments_cashout_impl(block_task_context& ctx)
        : _ctx(ctx)
        , dgp_service(ctx.services().dynamic_global_property_service())
        , account_service(ctx.services().account_service())
        , account_blogging_statistic_service(ctx.services().account_blogging_statistic_service())
        , comment_service(ctx.services().comment_service())
        , comment_statistic_service(ctx.services().comment_statistic_service())
        , comment_vote_service(ctx.services().comment_vote_service())
        , reward_fund_scr_service(ctx.services().reward_fund_scr_service())
        , reward_fund_sp_service(ctx.services().reward_fund_sp_service())
    {
    }

    void apply()
    {
        const auto fn = [&](const comment_object& c) { return c.cashout_time <= dgp_service.head_block_time(); };
        auto comments = comment_service.get_by_cashout_time(fn);

        apply_for_scr_fund(comments);
        apply_for_sp_fund(comments);

        for (const comment_object& comment : comments)
        {
            close_comment_payout(comment);
        }
    }

private:
    void apply_for_scr_fund(const comment_refs_type& comment)
    {
        if (reward_fund_scr_service.get().activity_reward_balance.amount > 0)
        {
            reward(reward_fund_scr_service, comment);
        }
    }
    void apply_for_sp_fund(const comment_refs_type& comment)
    {
        if (reward_fund_sp_service.get().activity_reward_balance.amount > 0)
        {
            reward(reward_fund_sp_service, comment);
        }
    }

    template <typename FundService> void reward(FundService& fund_service, const comment_refs_type& comments)
    {
        using fund_object_type = typename FundService::object_type;

        const auto& rf = fund_service.get();

        shares_vector_type total_rshares = get_total_rshares(comments);
        fc::uint128_t total_claims = rewards_math::calculate_total_claims(
            rf.recent_claims, dgp_service.head_block_time(), rf.last_update, rf.author_reward_curve, total_rshares,
            SCORUM_RECENT_RSHARES_DECAY_RATE);

        auto reward_symbol = rf.activity_reward_balance.symbol();
        asset reward = asset(0, reward_symbol);
        for (const comment_object& comment : comments)
        {
            if (comment.net_rshares > 0)
            {
                auto payout = rewards_math::calculate_payout(
                    comment.net_rshares, total_claims, rf.activity_reward_balance.amount, rf.author_reward_curve,
                    comment.max_accepted_payout.amount, SCORUM_MIN_COMMENT_PAYOUT_SHARE);
                reward += pay_for_comment(comment, asset(payout, reward_symbol));
            }
        }

        // Write the cached fund state back to the database
        fund_service.update([&](fund_object_type& rfo) {
            rfo.recent_claims = total_claims;
            rfo.activity_reward_balance -= reward;
            rfo.last_update = dgp_service.head_block_time();
        });
    }

    shares_vector_type get_total_rshares(const comment_service_i::comment_refs_type& comments)
    {
        shares_vector_type ret;

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
                asset curation_tokens = asset(rewards_math::calculate_curations_payout(reward.amount, SCORUM_CURATION_REWARD_PERCENT), reward_symbol);
                asset author_tokens = reward - curation_tokens;

                author_tokens += pay_curators(comment, curation_tokens); // curation_tokens can be changed inside pay_curators()

                asset claimed_reward = author_tokens + curation_tokens;

                auto total_beneficiary = asset(0, reward_symbol);
                for (auto& b : comment.beneficiaries)
                {
                    asset benefactor_tokens = (author_tokens * b.weight) / SCORUM_100_PERCENT;
                    pay_account(account_service.get_account(b.account), benefactor_tokens);
                    _ctx.push_virtual_operation(comment_benefactor_reward_operation(b.account, comment.author, fc::to_string(comment.permlink), benefactor_tokens));
                    total_beneficiary += benefactor_tokens;
                }

                author_tokens -= total_beneficiary;

                const auto& author = account_service.get_account(comment.author);
                pay_account(author, author_tokens);

                _ctx.push_virtual_operation(author_reward_operation(comment.author, fc::to_string(comment.permlink), author_tokens));
                _ctx.push_virtual_operation(comment_reward_operation(comment.author, fc::to_string(comment.permlink), claimed_reward));

                accumulate_statistic(comment, author, author_tokens, curation_tokens, total_beneficiary, reward_symbol);

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
                    auto claim = asset(rewards_math::calculate_curation_payout(max_rewards.amount, comment.total_vote_weight, vote.weight) , reward_symbol);
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

    void pay_account(const account_object &recipient, const asset &reward)
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
                              const asset &author_tokens, const asset &curation_tokens, const asset &total_beneficiary,
                              asset_symbol_type reward_symbol)
    {
        const auto &comment_stat = comment_statistic_service.get(comment.id);

        FC_ASSERT(author_tokens.symbol() == reward_symbol);
        FC_ASSERT(curation_tokens.symbol() == reward_symbol);
        FC_ASSERT(total_beneficiary.symbol() == reward_symbol);

        asset total_payout = author_tokens;
        total_payout += curation_tokens;
        total_payout += total_beneficiary;

        if (SCORUM_SYMBOL == reward_symbol)
        {
            comment_statistic_service.update(comment_stat, [&](comment_statistic_object& c) {
                c.total_payout_scr_value += total_payout;
                c.author_payout_scr_value += author_tokens;
                c.curator_payout_scr_value += curation_tokens;
                c.beneficiary_payout_scr_value += total_beneficiary;
            });
        }else if (SP_SYMBOL == reward_symbol)
        {
            comment_statistic_service.update(comment_stat, [&](comment_statistic_object& c) {
                c.total_payout_sp_value += total_payout;
                c.author_payout_sp_value += author_tokens;
                c.curator_payout_sp_value += curation_tokens;
                c.beneficiary_payout_sp_value += total_beneficiary;
            });
        }

#ifndef IS_LOW_MEM
        {
            const auto &author_stat = account_blogging_statistic_service.obtain(author.id);
            account_blogging_statistic_service.increase_posting_rewards(author_stat, author_tokens);
        }
#endif
    }

    void accumulate_statistic(const account_object &voter, const asset &curation_tokens)
    {
#ifndef IS_LOW_MEM
        const auto &voter_stat = account_blogging_statistic_service.obtain(voter.id);
        account_blogging_statistic_service.increase_curation_rewards(voter_stat, curation_tokens);
#endif
    }

private:

    block_task_context& _ctx;
    dynamic_global_property_service_i& dgp_service;
    account_service_i& account_service;
    account_blogging_statistic_service_i &account_blogging_statistic_service;
    comment_service_i& comment_service;
    comment_statistic_service_i& comment_statistic_service;
    comment_vote_service_i& comment_vote_service;
    reward_fund_scr_service_i& reward_fund_scr_service;
    reward_fund_sp_service_i& reward_fund_sp_service;
};

void process_comments_cashout::on_apply(block_task_context& ctx)
{
    process_comments_cashout_impl impl(ctx);

    impl.apply();
}

}
}
}


