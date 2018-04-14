
#include <scorum/rewards_math/curve.hpp>

namespace scorum {
namespace rewards {

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

uint128_t evaluate_reward_curve(const uint128_t& rshares, const curve_id& curve)
{
    uint128_t result = 0;

    switch (curve)
    {
    case curve_id::quadratic:
        result = rshares * rshares;
        break;
    case curve_id::linear:
        result = rshares;
        break;
    case curve_id::square_root:
        result = approx_sqrt(rshares);
        break;
    case curve_id::power1dot5:
        result = approx_sqrt(rshares * rshares * rshares);
        break;
    }

    return result;
}
}
} // scorum::rewards
