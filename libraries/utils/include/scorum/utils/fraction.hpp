#pragma once

#include <fc/safe.hpp>

namespace scorum {
namespace utils {

template <typename FractionalNumerator, typename FractionalDenominator> class fraction
{
    FractionalNumerator _numerator;
    FractionalDenominator _denominator;

public:
    fraction() = delete;
    fraction(const FractionalNumerator& numerator_, const FractionalDenominator& denominator_)
        : _numerator(numerator_) // copy
        , _denominator(denominator_) // copy
        , numerator(_numerator) // protect
        , denominator(_denominator) // protect
    {
    }

    const FractionalNumerator& numerator;
    const FractionalDenominator& denominator;
};

template <typename FractionalNumerator, typename FractionalDenominator>
fraction<FractionalNumerator, FractionalDenominator> make_fraction(const FractionalNumerator& numerator,
                                                                   const FractionalDenominator& denominator)
{
    return fraction<FractionalNumerator, FractionalDenominator>(numerator, denominator);
}

template <typename FractionalNumerator, typename FractionalDenominator>
fraction<FractionalNumerator, FractionalDenominator> make_fraction(const fc::safe<FractionalNumerator>& numerator,
                                                                   const FractionalDenominator& denominator)
{
    return fraction<FractionalNumerator, FractionalDenominator>(numerator.value, denominator);
}

template <typename FractionalNumerator, typename FractionalDenominator>
fraction<FractionalNumerator, FractionalDenominator> make_fraction(const FractionalNumerator& numerator,
                                                                   const fc::safe<FractionalDenominator>& denominator)
{
    return fraction<FractionalNumerator, FractionalDenominator>(numerator, denominator.value);
}

template <typename FractionalNumerator, typename FractionalDenominator>
fraction<FractionalNumerator, FractionalDenominator> make_fraction(const fc::safe<FractionalNumerator>& numerator,
                                                                   const fc::safe<FractionalDenominator>& denominator)
{
    return fraction<FractionalNumerator, FractionalDenominator>(numerator.value, denominator.value);
}
}
}
