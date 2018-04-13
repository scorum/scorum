#include <scorum/rewards/formulas.hpp>
#include <scorum/rewards/curve.hpp>

namespace scorum {
namespace rewards {

inline u256 to256(const fc::uint128& t)
{
    u256 v(t.hi);
    v <<= 64;
    v += t.lo;
    return v;
}

share_type predict_payout(const uint128_t& recent_claims,
                          const share_type& rshares,
                          const share_type& reward_fund,
                          curve_id reward_curve,
                          const share_type& max_share)
{
    uint128_t total_claims
        = calculate_total_claims(recent_claims, time_point_sec(), time_point_sec(), reward_curve, { rshares });
    return calculate_payout(rshares, total_claims, reward_fund, reward_curve, max_share);
}

uint128_t calculate_total_claims(const uint128_t& recent_claims,
                                 const time_point_sec& now,
                                 const time_point_sec& last_update,
                                 curve_id reward_curve,
                                 const share_types& vrshares)
{
    uint128_t total_claims;

    FC_ASSERT(now > last_update);

    fc::microseconds decay_rate = SCORUM_RECENT_RSHARES_DECAY_RATE;
    total_claims = recent_claims;
    total_claims -= (total_claims * (now - last_update).to_seconds()) / decay_rate.to_seconds();
    for (const share_type& rshares : vrshares)
    {
        total_claims += rewards::evaluate_reward_curve(rshares.value, reward_curve);
    }

    return total_claims;
}

share_type calculate_payout(const share_type& rshares,
                            const uint128_t& total_claims,
                            const share_type& reward_fund,
                            curve_id reward_curve,
                            const share_type& max_share)
{
    try
    {
        FC_ASSERT(rshares > 0);
        FC_ASSERT(total_claims > 0);

        u256 rf(reward_fund.value);
        u256 total_claims_ = to256(total_claims);

        u256 claim = to256(rewards::evaluate_reward_curve(rshares.value, reward_curve));

        u256 payout_u256 = (rf * claim) / total_claims_;
        FC_ASSERT(payout_u256 <= u256(uint64_t(std::numeric_limits<int64_t>::max())));
        uint64_t payout = static_cast<uint64_t>(payout_u256);

        if (payout < SCORUM_MIN_COMMENT_PAYOUT_SHARE)
            payout = 0;

        return std::min(payout, (uint64_t)max_share.value);
    }
    FC_CAPTURE_AND_RETHROW((rshares)(total_claims)(reward_fund)(reward_curve)(max_share))
}

share_type calculate_curations_payout(const share_type& payout)
{
    return (uint128_t(payout.value) * SCORUM_CURATION_REWARD_PERCENT / SCORUM_100_PERCENT).to_uint64();
}

share_type calculate_curation_payout(const share_type& curations_payout, uint64_t total_weight, uint64_t weight)
{
    share_type ret;
    uint128_t weight_(weight);
    ret = (weight_ * curations_payout.value / total_weight).to_uint64();
    return ret;
}
}
}
