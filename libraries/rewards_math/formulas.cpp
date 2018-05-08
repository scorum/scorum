#include <scorum/rewards_math/formulas.hpp>
#include <scorum/rewards_math/curve.hpp>

namespace scorum {
namespace rewards_math {

using scorum::protocol::share_value_type;

inline u256 to256(const fc::uint128& t)
{
    u256 v(t.hi);
    v <<= 64;
    v += t.lo;
    return v;
}

share_type predict_payout(const uint128_t& recent_claims,
                          const share_type& reward_fund,
                          const share_type& rshares,
                          const curve_id author_reward_curve,
                          const share_type& max_payout,
                          const fc::microseconds& decay_rate,
                          const share_type& min_comment_payout_share)
{
    uint128_t total_claims
        = calculate_decreasing_total_claims(recent_claims, time_point_sec(), time_point_sec(), decay_rate);
    total_claims = calculate_total_claims(total_claims, author_reward_curve, { rshares });
    return calculate_payout(rshares, total_claims, reward_fund, author_reward_curve, max_payout,
                            min_comment_payout_share);
}

uint128_t calculate_decreasing_total_claims(const uint128_t& recent_claims,
                                            const time_point_sec& now,
                                            const time_point_sec& last_payout_check,
                                            const fc::microseconds& decay_rate)
{
    try
    {
        FC_ASSERT(now >= last_payout_check);
        FC_ASSERT(decay_rate.to_seconds() > 0);

        uint128_t total_claims = recent_claims;
        int64_t decay_rate_s = decay_rate.to_seconds();
        int64_t delta = std::min((now - last_payout_check).to_seconds(), decay_rate_s);

        total_claims -= (total_claims * delta) / decay_rate_s;

        return total_claims;
    }
    FC_CAPTURE_AND_RETHROW((recent_claims)(now)(last_payout_check)(decay_rate))
}

uint128_t calculate_total_claims(const uint128_t& recent_claims,
                                 const curve_id author_reward_curve,
                                 const shares_vector_type& vrshares)
{
    try
    {
        uint128_t total_claims = recent_claims;

        for (const share_type& rshares : vrshares)
        {
            total_claims += evaluate_reward_curve(rshares.value, author_reward_curve);
        }

        return total_claims;
    }
    FC_CAPTURE_AND_RETHROW((recent_claims)(author_reward_curve)(vrshares))
}

share_type calculate_payout(const share_type& rshares,
                            const uint128_t& total_claims,
                            const share_type& reward_fund,
                            const curve_id author_reward_curve,
                            const share_type& max_share,
                            const share_type& min_comment_payout_share)
{
    try
    {
        FC_ASSERT(rshares > 0);
        FC_ASSERT(total_claims > 0);

        u256 rf(reward_fund.value);
        u256 total_claims_ = to256(total_claims);

        u256 claim = to256(evaluate_reward_curve(rshares.value, author_reward_curve));

        u256 payout_u256 = (rf * claim) / total_claims_;
        FC_ASSERT(payout_u256 <= u256(uint64_t(std::numeric_limits<int64_t>::max())));
        share_type payout = static_cast<share_value_type>(payout_u256);

        if (payout < min_comment_payout_share)
            payout = 0;

        return std::min(payout.value, max_share.value);
    }
    FC_CAPTURE_AND_RETHROW((rshares)(total_claims)(reward_fund)(author_reward_curve)(max_share))
}

share_type calculate_curations_payout(const share_type& payout, const percent_type scorum_curation_reward_percent)
{
    try
    {
        FC_ASSERT(SCORUM_100_PERCENT > 0);

        return (uint128_t(payout.value) * scorum_curation_reward_percent / SCORUM_100_PERCENT).to_uint64();
    }
    FC_CAPTURE_AND_RETHROW((payout))
}

share_type
calculate_curation_payout(const share_type& curations_payout, const uint64_t total_weight, const uint64_t weight)
{
    try
    {
        FC_ASSERT(total_weight > 0u);

        share_type ret;
        uint128_t weight_(weight);
        ret = (weight_ * curations_payout.value / total_weight).to_uint64();
        return ret;
    }
    FC_CAPTURE_AND_RETHROW((curations_payout)(total_weight)(weight))
}

/** this verifies uniqueness of voter
 *
 *  cv.weight / c.total_vote_weight ==> % of rshares increase that is accounted for by the vote
 *
 *  W(R) is bounded above by B. B is fixed at 2^64 - 1, so all weights fit in a 64 bit integer.
 *
 *  The equation for an individual vote is:
 *    W(R_N) - W(R_N-1), which is the delta increase of proportional weight
 *
 *  c.total_vote_weight =
 *    W(R_1) - W(R_0) +
 *    W(R_2) - W(R_1) + ...
 *    W(R_N) - W(R_N-1) = W(R_N) - W(R_0)
 *
 *  Since W(R_0) = 0, c.total_vote_weight is also bounded above by B and will always fit in a 64 bit
 *integer.
 *
 **/
uint64_t calculate_max_vote_weight(const share_type& positive_rshares,
                                   const share_type& recent_positive_rshares,
                                   const curve_id curation_reward_curve)
{
    try
    {
        FC_ASSERT(positive_rshares > recent_positive_rshares);

        uint64_t new_weight = evaluate_reward_curve(positive_rshares.value, curation_reward_curve).to_uint64();
        uint64_t old_weight = evaluate_reward_curve(recent_positive_rshares.value, curation_reward_curve).to_uint64();
        return new_weight - old_weight;
    }
    FC_CAPTURE_AND_RETHROW((positive_rshares)(recent_positive_rshares)(curation_reward_curve))
}

uint64_t calculate_vote_weight(const uint64_t max_vote_weight,
                               const time_point_sec& now,
                               const time_point_sec& when_comment_created,
                               const fc::microseconds& reverse_auction_window_seconds)
{
    try
    {
        FC_ASSERT(now >= when_comment_created);
        uint32_t reverse_auction_window_seconds_ = reverse_auction_window_seconds.to_seconds();
        FC_ASSERT(reverse_auction_window_seconds_ > 0);

        /// discount weight by time
        uint128_t w(max_vote_weight);
        uint64_t delta_t
            = std::min(uint64_t((now - when_comment_created).to_seconds()), uint64_t(reverse_auction_window_seconds_));

        w *= delta_t;
        w /= reverse_auction_window_seconds_;
        return w.to_uint64();
    }
    FC_CAPTURE_AND_RETHROW((max_vote_weight)(now)(when_comment_created))
}

percent_type calculate_restoring_power(const percent_type voting_power,
                                       const time_point_sec& now,
                                       const time_point_sec& last_voted,
                                       const fc::microseconds& vote_regeneration_seconds)
{
    try
    {
        FC_ASSERT(now >= last_voted);
        uint32_t vote_regeneration_seconds_ = vote_regeneration_seconds.to_seconds();
        FC_ASSERT(vote_regeneration_seconds_ > 0);

        int64_t elapsed_seconds = (now - last_voted).to_seconds();

        int64_t regenerated_power = (SCORUM_100_PERCENT * elapsed_seconds) / vote_regeneration_seconds_;
        return (percent_type)std::min(int64_t(voting_power) + regenerated_power, int64_t(SCORUM_100_PERCENT));
    }
    FC_CAPTURE_AND_RETHROW((voting_power)(now)(last_voted))
}

percent_type calculate_used_power(const percent_type voting_power,
                                  const vote_weight_type vote_weight,
                                  const percent_type decay_percent)
{
    try
    {
        FC_ASSERT(SCORUM_100_PERCENT > 0);
        FC_ASSERT(decay_percent > 0 && decay_percent < 100);

        int64_t abs_weight = std::abs(vote_weight);
        int64_t used_power = (voting_power * abs_weight) / SCORUM_100_PERCENT;

        used_power = (used_power * decay_percent) / 100;
        return (percent_type)used_power;
    }
    FC_CAPTURE_AND_RETHROW((voting_power)(vote_weight))
}

share_type calculate_abs_reward_shares(const percent_type used_voting_power, const share_type& effective_balance_shares)
{
    try
    {
        FC_ASSERT(SCORUM_100_PERCENT > 0);

        return ((uint128_t(effective_balance_shares.value) * used_voting_power) / (SCORUM_100_PERCENT)).to_uint64();
    }
    FC_CAPTURE_AND_RETHROW((used_voting_power)(effective_balance_shares))
}
}
}
