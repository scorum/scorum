#include <scorum/protocol/odds.hpp>

namespace scorum {
namespace protocol {

void odds::initialize(const odds_value_type& base_n, const odds_value_type& base_d)
{
    auto base = utils::make_fraction(base_n, base_d);
    _base = std::tie(base.numerator, base.denominator);
    auto simplified = base.simplify();
    _simplified = std::tie(simplified.numerator, simplified.denominator);
    auto inverted = simplified.invert();
    _inverted = std::tie(inverted.numerator, inverted.denominator);
}

odds_fraction_type odds::base() const
{
    return utils::make_fraction(std::get<0>(_base), std::get<1>(_base));
}

odds_fraction_type odds::simplified() const
{
    return utils::make_fraction(std::get<0>(_simplified), std::get<1>(_simplified));
}

odds_fraction_type odds::inverted() const
{
    return utils::make_fraction(std::get<0>(_inverted), std::get<1>(_inverted));
}

odds odds::from_string(const std::string& from)
{
    // read english format
    std::string s = fc::trim(from);
    auto slash_pos = s.find('/');
    FC_ASSERT(std::string::npos != slash_pos);
    auto n_str = s.substr(0, slash_pos);
    FC_ASSERT(!n_str.empty());
    auto d_str = s.substr(slash_pos + 1);
    FC_ASSERT(!d_str.empty());
    return utils::make_fraction((odds_value_type)fc::to_int64(n_str), (odds_value_type)fc::to_int64(d_str));
}

std::string odds::to_string() const
{
    auto f = base();
    std::string result = fc::to_string(f.numerator);
    result += '/';
    result += fc::to_string(f.denominator);
    return result;
}
}
}
