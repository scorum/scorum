#pragma once

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/types.hpp>

#include <vector>

namespace scorum {
namespace rewards_math {

using scorum::protocol::share_type;
using scorum::protocol::curve_id;
using scorum::protocol::percent_type;
using scorum::protocol::vote_weight_type;

using shares_vector_type = std::vector<share_type>;

share_type predict_payout(const uint128_t& recent_claims,
                          const share_type& reward_fund,
                          const share_type& rshares,
                          const curve_id author_reward_curve,
                          const share_type& max_payout,
                          const fc::microseconds& decay_rate,
                          const share_type& min_comment_payout_share);

uint128_t calculate_decreasing_total_claims(const uint128_t& recent_claims,
                                            const time_point_sec& now,
                                            const time_point_sec& last_payout_check,
                                            const fc::microseconds& decay_rate);

uint128_t calculate_total_claims(const uint128_t& recent_claims,
                                 const curve_id author_reward_curve,
                                 const shares_vector_type& vrshares);

share_type calculate_payout(const share_type& rshares,
                            const uint128_t& total_claims,
                            const share_type& reward_fund,
                            const curve_id author_reward_curve,
                            const share_type& max_share,
                            const share_type& min_comment_payout_share);

share_type calculate_curations_payout(const share_type& payout, const percent_type scorum_curation_reward_percent);

share_type
calculate_curation_payout(const share_type& curations_payout, const uint64_t total_weight, const uint64_t weight);

uint64_t calculate_max_vote_weight(const share_type& positive_rshares,
                                   const share_type& recent_positive_rshares,
                                   const curve_id curation_reward_curve);

uint64_t calculate_vote_weight(const uint64_t max_vote_weight,
                               const time_point_sec& now,
                               const time_point_sec& when_comment_created,
                               const fc::microseconds& reverse_auction_window_seconds);

// return voting power in SCORUM_PERCENTs
percent_type calculate_restoring_power(const percent_type voting_power,
                                       const time_point_sec& now,
                                       const time_point_sec& last_voted,
                                       const fc::microseconds& vote_regeneration_seconds);

time_point_sec calculate_expected_restoring_time(percent_type voting_power,
                                                 const time_point_sec& now,
                                                 const fc::microseconds& vote_regeneration_seconds);

percent_type calculate_used_power(const percent_type voting_power,
                                  const vote_weight_type vote_weight,
                                  const percent_type decay_percent);

share_type calculate_abs_reward_shares(const percent_type used_voting_power,
                                       const share_type& effective_balance_shares);
}
}
