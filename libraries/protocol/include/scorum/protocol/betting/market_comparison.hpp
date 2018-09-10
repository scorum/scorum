#include <scorum/utils/static_variant_comparison.hpp>

namespace scorum {
namespace protocol {
namespace betting {

template <market_kind kind, typename tag>
bool operator<(const over_under_market<kind, tag>& lhs, const over_under_market<kind, tag>& rhs)
{
    return lhs.threshold < rhs.threshold;
}

template <market_kind kind, typename tag>
bool operator<(const yes_no_market<kind, tag>& lhs, const yes_no_market<kind, tag>& rhs)
{
    return false;
}

template <market_kind kind, typename tag>
bool operator<(const score_yes_no_market<kind, tag>& lhs, const score_yes_no_market<kind, tag>& rhs)
{
    return std::tie(lhs.home, lhs.away) < std::tie(rhs.home, rhs.away);
}
}
}
}

namespace fc {
using scorum::protocol::betting::market_type;

template <> inline bool market_type::less_than(const market_type& that) const
{
    return scorum::utils::variant_less(*this, that);
}

template <> inline bool market_type::equal_to(const market_type& that) const
{
    return scorum::utils::variant_eq(*this, that);
}
}
