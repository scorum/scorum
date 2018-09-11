#pragma once
#include <cstdint>
#include <scorum/protocol/betting/market_kind.hpp>

namespace scorum {
namespace protocol {

template <bool site, market_kind kind, typename tag> struct over_under_wincase
{
    /// Market threshold x 1000 (i.e. -500 in 'handicap' market means -0.5)
    int16_t threshold;

    using opposite_type = over_under_wincase<!site, kind, tag>;
    static constexpr market_kind kind_v = kind;

    opposite_type create_opposite() const
    {
        return { threshold };
    }
};

template <bool site, market_kind kind, typename tag> struct score_yes_no_wincase
{
    uint16_t home;
    uint16_t away;

    using opposite_type = score_yes_no_wincase<!site, kind, tag>;
    static constexpr market_kind kind_v = kind;

    opposite_type create_opposite() const
    {
        return { home, away };
    }
};

template <bool site, market_kind kind, typename tag> struct yes_no_wincase
{
    using opposite_type = yes_no_wincase<!site, kind, tag>;
    static constexpr market_kind kind_v = kind;

    opposite_type create_opposite() const
    {
        return {};
    }
};
}
}
