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
        FC_ASSERT(denominator_ != (FractionalDenominator)0, "Division by zero");
    }

    fraction(const fraction<FractionalNumerator, FractionalDenominator>& other)
        : fraction(other.numerator, other.denominator)
    {
    }

    template <typename OtherFractionalNumerator, typename OtherFractionalDenominator>
    friend bool operator==(const fraction<FractionalNumerator, FractionalDenominator>& a,
                           const fraction<OtherFractionalNumerator, OtherFractionalDenominator>& b)
    {
        return a.numerator == (FractionalNumerator)b.numerator && a.denominator == (FractionalDenominator)b.denominator;
    }

    template <typename OtherFractionalNumerator, typename OtherFractionalDenominator>
    friend bool operator/(const fraction<FractionalNumerator, FractionalDenominator>& a,
                          const fraction<OtherFractionalNumerator, OtherFractionalDenominator>& b);

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
    auto denominator_ = denominator;
    auto numerator_ = numerator;
    if (denominator_ < 0)
    {
        denominator_ = -denominator_;
        if (numerator_ < 0)
            numerator_ = -numerator_;
    }
    return make_fraction<FractionalNumerator, FractionalDenominator>(denominator_ - numerator_, denominator_);
}

template <typename FractionalNumerator, typename FractionalDenominator>
auto operator/(const fraction<FractionalNumerator, FractionalDenominator>& a,
               const fraction<FractionalNumerator, FractionalDenominator>& b)
{
    using large_type = int64_t;

    FC_ASSERT(sizeof(FractionalNumerator) < sizeof(large_type));
    FC_ASSERT(sizeof(FractionalDenominator) < sizeof(large_type));

    using safe_type = fc::safe<large_type>;
    safe_type n = a.numerator;
    n *= b.denominator;
    safe_type d = a.denominator;
    d *= b.numerator;

    fraction<large_type, large_type> tmp(make_fraction(n, d).simplify());

    return make_fraction<FractionalNumerator, FractionalDenominator>(tmp.numerator, tmp.denominator);
}

template <typename Stream, typename FractionalNumerator, typename FractionalDenominator>
Stream& operator<<(Stream& stream, const fraction<FractionalNumerator, FractionalDenominator>& f)
{
    stream << f.numerator << '/' << f.denominator;
    return stream;
}
}
}
