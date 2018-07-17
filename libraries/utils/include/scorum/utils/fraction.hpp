#pragma once

#include <fc/safe.hpp>

namespace scorum {
namespace utils {

template <typename FractionalNumerator, typename FractionalDenominator> class fraction
{
public:
    fraction() = delete;
    fraction(const FractionalNumerator& numerator_, const FractionalDenominator& denominator_)
        : numerator(numerator_)
        , denominator(denominator_)
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
