#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_statistic.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/account_blogging_statistic.hpp>
#include <scorum/chain/services/hardfork_property.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/reward_objects.hpp>

#include <scorum/rewards_math/curve.hpp>
#include <scorum/rewards_math/formulas.hpp>

#include <boost/range/adaptor/reversed.hpp>
#include <map>

namespace scorum {
namespace chain {
namespace database_ns {

using scorum::rewards_math::shares_vector_type;
using comment_refs_type = scorum::chain::comment_service_i::comment_refs_type;

class process_comments_cashout_impl
{
public:
    struct comment_payout_result
    {
        /// amount of tokens distributed within particular comment across author, beneficiars and curators
        asset total_claimed_reward;
        asset parent_comment_reward;
    };

    explicit process_comments_cashout_impl(block_task_context& ctx);

    template <typename FundService> void update_decreasing_total_claims(FundService& fund_service)
    {
        using fund_object_type = typename FundService::object_type;

        const auto& rf = fund_service.get();

        auto now = dgp_service.head_block_time();

        fc::uint128_t total_claims = rewards_math::calculate_decreasing_total_claims(
            rf.recent_claims, now, rf.last_update, SCORUM_RECENT_RSHARES_DECAY_RATE);

        fund_service.update([&](fund_object_type& rfo) {
            rfo.recent_claims = total_claims;
            rfo.last_update = now;
        });
    }

    template <typename TFundService> void reward(TFundService& fund_service, const comment_refs_type& comments);

    asset pay_for_comments(const comment_refs_type& comments, const std::vector<asset>& fund_rewards);

    comment_payout_result pay_for_comment(const comment_object& comment,
                                          const asset& publication_reward,
                                          const asset& children_comments_reward);

    void close_comment_payout(const comment_object& comment);

private:
    std::vector<asset> calculate_comments_payout(const comment_refs_type& comments,
                                                 const asset& reward_fund_balance,
                                                 fc::uint128_t total_claims,
                                                 curve_id reward_curve) const;

    fc::uint128_t
    get_total_claims(const comment_refs_type& comments, curve_id reward_curve, fc::uint128_t recent_claims) const;

    asset pay_curators(const comment_object& comment, asset& max_rewards);

    void pay_account(const account_object& recipient, const asset& reward);

    template <class CommentStatisticService>
    void accumulate_comment_statistic(CommentStatisticService& stat_service,
                                      const comment_object& comment,
                                      const asset& total_payout,
                                      const asset& author_tokens,
                                      const asset& curation_tokens,
                                      const asset& total_beneficiary,
                                      const asset& publication_tokens,
                                      const asset& children_comments_tokens)
    {
        using comment_object_type = typename CommentStatisticService::object_type;

        const auto& stat = stat_service.get(comment.id);
        stat_service.update(stat, [&](comment_object_type& c) {
            c.total_payout_value += total_payout;
            c.author_payout_value += author_tokens;
            c.curator_payout_value += curation_tokens;
            c.beneficiary_payout_value += total_beneficiary;
            c.comment_publication_reward += publication_tokens;
            c.children_comments_reward += children_comments_tokens;
        });
    }

    void accumulate_statistic(const comment_object& comment,
                              const account_object& author,
                              const asset& author_tokens,
                              const asset& curation_tokens,
                              const asset& total_beneficiary,
                              const asset& publication_tokens,
                              const asset& children_comments_tokens,
                              asset_symbol_type reward_symbol);

    void accumulate_statistic(const account_object& voter, const asset& curation_tokens);

    comment_refs_type collect_parents(const comment_refs_type& comments);

    fc::shared_string get_permlink(const fc::shared_string& str) const;

private:
    block_task_context& _ctx;
    dynamic_global_property_service_i& dgp_service;
    account_service_i& account_service;
    account_blogging_statistic_service_i& account_blogging_statistic_service;
    comment_service_i& comment_service;
    comment_statistic_scr_service_i& comment_statistic_scr_service;
    comment_statistic_sp_service_i& comment_statistic_sp_service;
    comment_vote_service_i& comment_vote_service;
    hardfork_property_service_i& hardfork_service;
};
}
}
}
