#include <scorum/utils/static_variant_comparison.hpp>

namespace scorum {
namespace protocol {

template <bool side, market_kind kind, typename tag>
bool operator<(const over_under_wincase<side, kind, tag>& lhs, const over_under_wincase<side, kind, tag>& rhs)
{
    return lhs.threshold < rhs.threshold;
}

template <bool side, market_kind kind, typename tag>
bool operator<(const yes_no_wincase<side, kind, tag>& lhs, const yes_no_wincase<side, kind, tag>& rhs)
{
    return false;
}

template <bool side, market_kind kind, typename tag>
bool operator<(const score_yes_no_wincase<side, kind, tag>& lhs, const score_yes_no_wincase<side, kind, tag>& rhs)
{
    return std::tie(lhs.home, lhs.away) < std::tie(rhs.home, rhs.away);
}
}
}

namespace fc {
using scorum::protocol::wincase_type;

template <> inline bool wincase_type::less_than(const wincase_type& that) const
{
    return scorum::utils::variant_less(*this, that);
}

template <> inline bool wincase_type::equal_to(const wincase_type& that) const
{
    return scorum::utils::variant_eq(*this, that);
}
}
