#include <scorum/protocol/betting/market.hpp>

namespace scorum {
namespace protocol {

struct wincases_builder
{
    template <market_kind kind, typename tag>
    std::pair<wincase_type, wincase_type> operator()(const over_under_market<kind, tag>& m) const
    {
        return { over_under_wincase<true, kind, tag>{ m.threshold },
                 over_under_wincase<false, kind, tag>{ m.threshold } };
    }

    template <market_kind kind, typename tag>
    std::pair<wincase_type, wincase_type> operator()(const score_yes_no_market<kind, tag>& m) const
    {
        return { score_yes_no_wincase<true, kind, tag>{ m.home, m.away },
                 score_yes_no_wincase<false, kind, tag>{ m.home, m.away } };
    }

    template <market_kind kind, typename tag>
    std::pair<wincase_type, wincase_type> operator()(const yes_no_market<kind, tag>& m) const
    {
        return { yes_no_wincase<true, kind, tag>{}, yes_no_wincase<false, kind, tag>{} };
    }
};

struct market_builder
{
    template <bool site, market_kind kind, typename tag>
    market_type operator()(const over_under_wincase<site, kind, tag>& w) const
    {
        return over_under_market<kind, tag>{ w.threshold };
    }

    template <bool site, market_kind kind, typename tag>
    market_type operator()(const score_yes_no_wincase<site, kind, tag>& w) const
    {
        return score_yes_no_market<kind, tag>{ w.home, w.away };
    }

    template <bool site, market_kind kind, typename tag>
    market_type operator()(const yes_no_wincase<site, kind, tag>& w) const
    {
        return yes_no_market<kind, tag>{};
    }
};

std::pair<wincase_type, wincase_type> create_wincases(const market_type& market)
{
    return market.visit(wincases_builder{});
}

market_type create_market(const wincase_type& wincase)
{
    return wincase.visit(market_builder{});
}

wincase_type create_opposite(const wincase_type& wincase)
{
    return wincase.visit([](const auto& w) { return wincase_type(w.create_opposite()); });
}

bool has_trd_state(const market_type& market)
{
    return market.visit([](const auto& m) { return m.has_trd_state(); });
}

bool match_wincases(const wincase_type& lhs, const wincase_type& rhs)
{
    auto lhs_opposite = create_opposite(lhs);

    return !(lhs_opposite < rhs) && !(rhs < lhs_opposite);
}
}
}
