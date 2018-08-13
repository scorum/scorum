#pragma once

#include <scorum/utils/fraction.hpp>

#include <tuple>

namespace scorum {
namespace protocol {

using odds_value_type = int16_t;
using odds_fraction_type = utils::fraction<odds_value_type, odds_value_type>;

class odds
{
    void initialize(const odds_value_type& base_n, const odds_value_type& base_d);

public:
    odds() = default;

    odds(const odds_value_type& base_n, const odds_value_type& base_d)
    {
        initialize(base_n, base_d);
    }

    template <typename FractionalNumerator, typename FractionalDenominator>
    odds(const utils::fraction<FractionalNumerator, FractionalDenominator>& base)
    {
        if ((sizeof(FractionalNumerator) > sizeof(odds_value_type))
            && (base.numerator > (FractionalNumerator)std::numeric_limits<odds_value_type>::max()
                || base.numerator < (FractionalNumerator)std::numeric_limits<odds_value_type>::min()))
        {
            FC_CAPTURE_AND_THROW(fc::overflow_exception, (base.numerator));
        }
        if ((sizeof(FractionalDenominator) > sizeof(odds_value_type))
            && (base.denominator > (FractionalDenominator)std::numeric_limits<odds_value_type>::max()
                || base.denominator < (FractionalDenominator)std::numeric_limits<odds_value_type>::min()))
        {
            FC_CAPTURE_AND_THROW(fc::overflow_exception, (base.denominator));
        }
        initialize((odds_value_type)base.numerator, (odds_value_type)base.denominator);
    }

    friend bool operator==(const odds& a, const odds& b)
    {
        return a._simplified == b._simplified;
    }

    odds_fraction_type base() const;

    odds_fraction_type simplified() const;

    odds_fraction_type inverted() const;

    operator odds_fraction_type() const
    {
        return simplified();
    }

    operator bool() const
    {
        return _simplified != std::tuple<odds_value_type, odds_value_type>();
    }

    static odds from_string(const std::string& from);
    std::string to_string() const;

private:
    std::tuple<odds_value_type, odds_value_type> _base;
    std::tuple<odds_value_type, odds_value_type> _simplified;
    std::tuple<odds_value_type, odds_value_type> _inverted;
};

template <typename Stream> Stream& operator<<(Stream& stream, const scorum::protocol::odds& o)
{
    stream << o.to_string();
    return stream;
}

template <typename Stream> Stream& operator>>(Stream& stream, scorum::protocol::odds& o)
{
    std::string str;
    stream >> str;
    o = scorum::protocol::odds::from_string(str);
    return stream;
}
} // namespace protocol
} // namespace scorum

namespace fc {

inline void to_variant(const scorum::protocol::odds& var, fc::variant& vo)
{
    vo = var.to_string();
}

inline void from_variant(const fc::variant& var, scorum::protocol::odds& vo)
{
    vo = scorum::protocol::odds::from_string(var.as_string());
}

} // namespace fc

FC_REFLECT_EMPTY(scorum::protocol::odds)
