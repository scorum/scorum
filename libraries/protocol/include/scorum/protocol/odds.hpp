#pragma once

#include <scorum/utils/fraction.hpp>

#include <tuple>

namespace scorum {
namespace protocol {

using odds_value_type = int16_t;
using odds_fraction_type = utils::fraction<odds_value_type, odds_value_type>;

class odds
{

public:
    odds(const odds_fraction_type& base);

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

    static odds from_string(const std::string& from);
    std::string to_string() const;

private:
    std::tuple<odds_value_type, odds_value_type> _base;
    std::tuple<odds_value_type, odds_value_type> _simplified;
    std::tuple<odds_value_type, odds_value_type> _inverted;
};

template <typename Stream> Stream& operator<<(Stream& stream, const scorum::protocol::odds& a)
{
    stream << a.to_string();
    return stream;
}

template <typename Stream> Stream& operator>>(Stream& stream, scorum::protocol::odds& a)
{
    std::string str;
    stream >> str;
    a = scorum::protocol::odds::from_string(str);
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
