#include <scorum/protocol/odds.hpp>

namespace scorum {
namespace protocol {

void odds::initialize(const odds_value_type& base_n, const odds_value_type& base_d)
{
    FC_ASSERT(base_n > 0, "Numerator must be positive and non zero");
    FC_ASSERT(base_d > 0, "Denominator must be positive and non zero");
    FC_ASSERT(base_n > base_d, "Numerator must be more then denominator (inverted probability = (0, 1))");
    auto base = utils::make_fraction(base_n, base_d);
    _base = std::tie(base.numerator, base.denominator);
    auto simplified = base.simplify();
    _simplified = std::tie(simplified.numerator, simplified.denominator);
    auto inverted = simplified.coup().invert();
    _inverted = std::tie(inverted.denominator, inverted.numerator);
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
    auto n = fc::to_int64(n_str);
    FC_ASSERT(n > 0 && n <= (int64_t)std::numeric_limits<odds_value_type>::max());

    auto d_str = s.substr(slash_pos + 1);
    FC_ASSERT(!d_str.empty());
    auto d = fc::to_int64(d_str);
    FC_ASSERT(d > 0 && d <= (int64_t)std::numeric_limits<odds_value_type>::max());

    return utils::make_fraction((odds_value_type)n, (odds_value_type)d);
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

namespace fc {
void to_variant(const scorum::protocol::odds& var, fc::variant& vo)
{
    auto base = var.base();
    vo = fc::mutable_variant_object()("numerator", base.numerator)("denominator", base.denominator);
}

void from_variant(const fc::variant& var, scorum::protocol::odds& vo)
{
    using namespace scorum::protocol;

    const auto& variant_obj = var.get_object();
    auto numerator = variant_obj["numerator"].as<odds_value_type>();
    auto denominator = variant_obj["denominator"].as<odds_value_type>();

    vo = scorum::protocol::odds{ numerator, denominator };
}
} // namespace fc
