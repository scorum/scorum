
#include <scorum/chain/util/reward.hpp>
#include <scorum/chain/util/uint256.hpp>

namespace scorum {
namespace chain {
namespace util {

uint8_t find_msb(const uint128_t& u)
{
    uint64_t x;
    uint8_t places;
    x = (u.lo ? u.lo : 1);
    places = (u.hi ? 64 : 0);
    x = (u.hi ? u.hi : x);
    return uint8_t(boost::multiprecision::detail::find_msb(x) + places);
}

uint64_t approx_sqrt(const uint128_t& x)
{
    if ((x.lo == 0) && (x.hi == 0))
        return 0;

    uint8_t msb_x = find_msb(x);
    uint8_t msb_z = msb_x >> 1;

    uint128_t msb_x_bit = uint128_t(1) << msb_x;
    uint64_t msb_z_bit = uint64_t(1) << msb_z;

    uint128_t mantissa_mask = msb_x_bit - 1;
    uint128_t mantissa_x = x & mantissa_mask;
    uint64_t mantissa_z_hi = (msb_x & 1) ? msb_z_bit : 0;
    uint64_t mantissa_z_lo = (mantissa_x >> (msb_x - msb_z)).lo;
    uint64_t mantissa_z = (mantissa_z_hi | mantissa_z_lo) >> 1;
    uint64_t result = msb_z_bit | mantissa_z;

    return result;
}

asset get_rshare_reward(const comment_reward_context& ctx)
{
    try
    {
        FC_ASSERT(ctx.rshares > 0);
        FC_ASSERT(ctx.total_reward_shares2 > 0);

        u256 rf(ctx.total_reward_fund_scorum.amount.value);
        u256 total_claims = to256(ctx.total_reward_shares2);

        // idump( (ctx) );

        u256 claim = to256(evaluate_reward_curve(ctx.rshares.value, ctx.reward_curve));
        claim = (claim * ctx.reward_weight) / SCORUM_100_PERCENT;

        u256 payout_u256 = (rf * claim) / total_claims;
        FC_ASSERT(payout_u256 <= u256(uint64_t(std::numeric_limits<int64_t>::max())));
        uint64_t payout = static_cast<uint64_t>(payout_u256);

        if (is_comment_payout_dust(payout))
            payout = 0;

        return std::min(asset(payout, ctx.max_scr.symbol()), ctx.max_scr);
    }
    FC_CAPTURE_AND_RETHROW((ctx))
}

uint128_t evaluate_reward_curve(const uint128_t& rshares, const curve_id& curve)
{
    uint128_t result = 0;

    switch (curve)
    {
    case quadratic:
        result = rshares * rshares;
        break;
    case linear:
        result = rshares;
        break;
    case square_root:
        result = approx_sqrt(rshares);
        break;
    case power1dot5:
        result = approx_sqrt(rshares * rshares * rshares);
        break;
    }

    return result;
}
}
}
} // scorum::chain::util
