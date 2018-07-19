#pragma once

#include <fc/uint128.hpp>
#include <fc/exception/exception.hpp>

namespace scorum {
namespace utils {

template <typename ModifyingValue, typename FractionalNumerator, typename FractionalDenominator>
ModifyingValue multiply_by_fractional(const ModifyingValue& val,
                                      const FractionalNumerator& numerator,
                                      const FractionalDenominator& denominator)
{
    try
    {
        FC_ASSERT(val >= static_cast<ModifyingValue>(0), "Only unsigned value accepted");
        FC_ASSERT(numerator >= static_cast<FractionalNumerator>(0), "Only unsigned numerator accepted");
        FC_ASSERT(denominator > static_cast<FractionalDenominator>(0),
                  "Only unsigned and non zero denominator accepted");
        fc::uint128_t extra_hi = val;
        extra_hi *= numerator;
        extra_hi /= denominator;
        return static_cast<ModifyingValue>(extra_hi.to_uint64());
    }
    FC_CAPTURE_AND_RETHROW((val)(numerator)(denominator))
}
}
}
