#include <scorum/chain/evaluators/vote_evaluator.hpp>

#include <scorum/chain/data_service_factory_def.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account_blogging_statistic.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/hardfork_property.hpp>

#include <scorum/rewards_math/formulas.hpp>

namespace scorum {
namespace chain {

vote_evaluator::vote_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, vote_evaluator>(services)
    , _account_service(services.account_service())
    , _comment_service(services.comment_service())
    , _comment_vote_service(services.comment_vote_service())
    , _dgp_service(services.dynamic_global_property_service())
    , _hardfork_service(services.hardfork_property_service())
{
}

vote_weight_type vote_evaluator::get_weigth(const vote_evaluator::operation_type& o) const
{
    if (_hardfork_service.has_hardfork(SCORUM_HARDFORK_0_2))
    {
        FC_ASSERT(abs(o.weight) <= SCORUM_100_PERCENT, "Weight is not a SCORUM percentage");
        return o.weight;
    }
    else
    {
        FC_ASSERT(abs(o.weight) <= 100, "Weight is not a SCORUM percentage");
        return static_cast<vote_weight_type>(o.weight * SCORUM_1_PERCENT);
    }
}

void vote_evaluator::do_apply(const operation_type& o)
{
    try
    {
        const auto& comment = _comment_service.get(o.author, o.permlink);
        const auto& voter = _account_service.get_account(o.voter);

        FC_ASSERT(!(voter.owner_challenged || voter.active_challenged),
                  "Operation cannot be processed because the account is currently challenged.");

        FC_ASSERT(voter.can_vote, "Voter has declined their voting rights.");

        const vote_weight_type weight = get_weigth(o);

        if (weight > 0)
            FC_ASSERT(comment.allow_votes, "Votes are not allowed on the comment.");

        if (comment.cashout_time == fc::time_point_sec::maximum())
        {
#ifndef CLEAR_VOTES
            if (!_comment_vote_service.is_exists(comment.id, voter.id))
            {
                _comment_vote_service.create([&](comment_vote_object& cvo) {
                    cvo.voter = voter.id;
                    cvo.comment = comment.id;
                    cvo.vote_percent = weight;
                    cvo.last_update = _dgp_service.head_block_time();
                });
            }
            else
            {
                const comment_vote_object& comment_vote = _comment_vote_service.get(comment.id, voter.id);
                _comment_vote_service.update(comment_vote, [&](comment_vote_object& cvo) {
                    cvo.vote_percent = weight;
                    cvo.last_update = _dgp_service.head_block_time();
                });
            }
#endif
            return;
        }

        {
            int64_t elapsed_seconds = (_dgp_service.head_block_time() - voter.last_vote_time).to_seconds();
            FC_ASSERT(elapsed_seconds >= SCORUM_MIN_VOTE_INTERVAL_SEC, "Can only vote once every 3 seconds.");
        }

        uint16_t current_power = rewards_math::calculate_restoring_power(
            voter.voting_power, _dgp_service.head_block_time(), voter.last_vote_time, SCORUM_VOTE_REGENERATION_SECONDS);
        FC_ASSERT(current_power > 0, "Account currently does not have voting power.");

        uint16_t used_power
            = rewards_math::calculate_used_power(current_power, weight, SCORUM_VOTING_POWER_DECAY_PERCENT);

        FC_ASSERT(used_power <= current_power, "Account does not have enough power to vote.");

        share_type abs_rshares
            = rewards_math::calculate_abs_reward_shares(used_power, voter.effective_scorumpower().amount);

        FC_ASSERT(abs_rshares > SCORUM_VOTE_DUST_THRESHOLD || weight == 0,
                  "Voting weight is too small, please accumulate more voting power or scorum power.");

        /// this is the rshares voting for or against the post
        share_type rshares = weight < 0 ? -abs_rshares : abs_rshares;

        if (!_comment_vote_service.is_exists(comment.id, voter.id))
        {
            FC_ASSERT(weight != 0, "Vote weight cannot be 0.");

            if (rshares > 0)
            {
                FC_ASSERT(_dgp_service.head_block_time() < comment.cashout_time - SCORUM_UPVOTE_LOCKOUT,
                          "Cannot increase payout within last twelve hours before payout.");
            }

            _account_service.update_voting_power(voter, current_power - used_power);

            const auto& root_comment = _comment_service.get(comment.root_comment);

            FC_ASSERT(abs_rshares > 0, "Cannot vote with 0 rshares.");

            auto old_vote_rshares = comment.vote_rshares;

            _comment_service.update(comment, [&](comment_object& c) {
                c.net_rshares += rshares;
                c.abs_rshares += abs_rshares;
                if (rshares > 0)
                    c.vote_rshares += rshares;
                if (rshares > 0)
                    c.net_votes++;
                else
                    c.net_votes--;
            });

            _comment_service.update(root_comment, [&](comment_object& c) { c.children_abs_rshares += abs_rshares; });

            uint64_t max_vote_weight = 0;

            _comment_vote_service.create([&](comment_vote_object& cv) {
                cv.voter = voter.id;
                cv.comment = comment.id;
                cv.rshares = rshares;
                cv.vote_percent = weight;
                cv.last_update = _dgp_service.head_block_time();

                bool curation_reward_eligible
                    = rshares > 0 && (comment.last_payout == fc::time_point_sec()) && comment.allow_curation_rewards;

                if (curation_reward_eligible)
                {
                    const auto& reward_fund = db().content_reward_fund_scr_service().get();
                    max_vote_weight = rewards_math::calculate_max_vote_weight(comment.vote_rshares, old_vote_rshares,
                                                                              reward_fund.curation_reward_curve);
                    cv.weight = rewards_math::calculate_vote_weight(max_vote_weight, cv.last_update, comment.created,
                                                                    SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);
                }
                else
                {
                    cv.weight = 0;
                }
            });

#ifndef IS_LOW_MEM
            {
                account_blogging_statistic_service_i& account_blogging_statistic_service
                    = db().account_blogging_statistic_service();

                const auto& voter_stat = account_blogging_statistic_service.obtain(voter.id);
                account_blogging_statistic_service.add_vote(voter_stat);
            }
#endif

            if (max_vote_weight) // Optimization
            {
                _comment_service.update(comment, [&](comment_object& c) { c.total_vote_weight += max_vote_weight; });
            }
        }
        else
        {
            const comment_vote_object& comment_vote = _comment_vote_service.get(comment.id, voter.id);

            FC_ASSERT(comment_vote.num_changes != -1, "Cannot vote again on a comment after payout.");

            FC_ASSERT(comment_vote.num_changes < SCORUM_MAX_VOTE_CHANGES,
                      "Voter has used the maximum number of vote changes on this comment.");

            FC_ASSERT(comment_vote.vote_percent != weight, "You have already voted in a similar way.");

            if (comment_vote.rshares < rshares)
            {
                FC_ASSERT(_dgp_service.head_block_time() < comment.cashout_time - SCORUM_UPVOTE_LOCKOUT,
                          "Cannot increase payout within last twelve hours before payout.");
            }

            _account_service.update_voting_power(voter, current_power - used_power);

            const auto& root_comment = _comment_service.get(comment.root_comment);

            _comment_service.update(comment, [&](comment_object& c) {
                c.net_rshares -= comment_vote.rshares;
                c.net_rshares += rshares;
                c.abs_rshares += abs_rshares;

                /// TODO: figure out how to handle remove a vote (rshares == 0 )
                if (rshares > 0 && comment_vote.rshares < 0)
                    c.net_votes += 2;
                else if (rshares > 0 && comment_vote.rshares == 0)
                    c.net_votes += 1;
                else if (rshares == 0 && comment_vote.rshares < 0)
                    c.net_votes += 1;
                else if (rshares == 0 && comment_vote.rshares > 0)
                    c.net_votes -= 1;
                else if (rshares < 0 && comment_vote.rshares == 0)
                    c.net_votes -= 1;
                else if (rshares < 0 && comment_vote.rshares > 0)
                    c.net_votes -= 2;
            });

            _comment_service.update(root_comment, [&](comment_object& c) { c.children_abs_rshares += abs_rshares; });

            _comment_service.update(comment, [&](comment_object& c) { c.total_vote_weight -= comment_vote.weight; });

            _comment_vote_service.update(comment_vote, [&](comment_vote_object& cv) {
                cv.rshares = rshares;
                cv.vote_percent = weight;
                cv.last_update = _dgp_service.head_block_time();
                cv.weight = 0;
                cv.num_changes += 1;
            });
        }
    }
    FC_CAPTURE_AND_RETHROW()
}

} // namespace chain
} // namespace scorum
