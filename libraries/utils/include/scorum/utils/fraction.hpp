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
        FC_ASSERT(denominator_ > 0, "Division by zero");
    }

    friend bool operator==(const fraction<FractionalNumerator, FractionalDenominator>& a,
                           const fraction<FractionalNumerator, FractionalDenominator>& b)
    {
        return a.numerator == b.numerator && a.denominator == b.denominator;
    }

    //(n/gcd)/(d/gcd)
    fraction<FractionalNumerator, FractionalDenominator> simplify() const;

    //(d-n)/d
    fraction<FractionalNumerator, FractionalDenominator> invert() const;

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

template <typename FractionalNumerator, typename FractionalDenominator>
fraction<FractionalNumerator, FractionalDenominator>
fraction<FractionalNumerator, FractionalDenominator>::simplify() const
{
    // calculate GCD (Greatest Common Divisor)
    auto n = numerator;
    auto d = denominator;
    if (n < 0)
        n = -n;
    if (d < 0)
        d = -d;
    FractionalNumerator gcd = 0;
    while (d != 0)
    {
        n %= d;
        if (n == 0)
        {
            gcd = d;
            break;
        }
        d %= n;
    }
    if (gcd == 0)
        gcd = n;
    if (gcd != 0)
    {
        // simplify
        return make_fraction<FractionalNumerator, FractionalDenominator>(numerator / gcd, denominator / gcd);
    }

    return (*this);
}

template <typename FractionalNumerator, typename FractionalDenominator>
fraction<FractionalNumerator, FractionalDenominator>
fraction<FractionalNumerator, FractionalDenominator>::invert() const
{
    return make_fraction<FractionalNumerator, FractionalDenominator>(denominator - numerator, denominator);
}
}
}
