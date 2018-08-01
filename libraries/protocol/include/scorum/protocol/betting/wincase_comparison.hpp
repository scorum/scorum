#pragma once
#include <scorum/protocol/betting/wincase.hpp>

namespace scorum {
namespace protocol {
namespace betting {
// clang-format off
template <market_kind kind, typename tag>
bool operator<(const over<kind, tag>& lhs, const over<kind, tag>& rhs)
{
    return lhs.threshold < rhs.threshold;
}

template <market_kind kind, typename tag>
bool operator<(const under<kind, tag>& lhs, const under<kind, tag>& rhs)
{
    return lhs.threshold < rhs.threshold;
}

template <market_kind kind, typename tag>
bool operator<(const yes<kind, tag>& lhs, const yes<kind, tag>& rhs)
{
    return false;
}

template <market_kind kind, typename tag>
bool operator<(const no<kind, tag>& lhs, const no<kind, tag>& rhs)
{
    return false;
}

template <market_kind kind, typename tag>
bool operator<(const score_yes<kind, tag>& lhs, const score_yes<kind, tag>& rhs)
{
    return std::tie(lhs.home, lhs.away) < std::tie(rhs.home, rhs.away);
}

template <market_kind kind, typename tag>
bool operator<(const score_no<kind, tag>& lhs, const score_no<kind, tag>& rhs)
{
    return std::tie(lhs.home, lhs.away) < std::tie(rhs.home, rhs.away);
}

template <typename TWinCase>
bool operator<(const TWinCase& lhs, const wincase_type& rhs)
{
    auto tagl = wincase_type::tag<TWinCase>::value;
    auto tagr = rhs.which();

    return tagl < tagr || (!(tagr < tagl) && lhs < rhs.get<TWinCase>());
}

template <typename TWinCase>
bool operator<(const wincase_type& lhs, const TWinCase& rhs)
{
    auto tagl = lhs.which();
    auto tagr = wincase_type::tag<TWinCase>::value;

    return tagl < tagr || (!(tagr < tagl) && lhs.get<std::decay_t<decltype(rhs)>>() < rhs);
}

template <market_kind kind, typename tag>
bool operator==(const over<kind, tag>& lhs, const over<kind, tag>& rhs)
{
    return lhs.threshold.value == rhs.threshold.value;
}

template <market_kind kind, typename tag>
bool operator==(const under<kind, tag>& lhs, const under<kind, tag>& rhs)
{
    return lhs.threshold.value == rhs.threshold.value;
}

template <market_kind kind, typename tag>
bool operator==(const yes<kind, tag>& lhs, const yes<kind, tag>& rhs)
{
    return true;
}

template <market_kind kind, typename tag>
bool operator==(const no<kind, tag>& lhs, const no<kind, tag>& rhs)
{
    return true;
}

template <market_kind kind, typename tag>
bool operator==(const score_yes<kind, tag>& lhs, const score_yes<kind, tag>& rhs)
{
    return std::tie(lhs.home, lhs.away) == std::tie(rhs.home, rhs.away);
}

template <market_kind kind, typename tag>
bool operator==(const score_no<kind, tag>& lhs, const score_no<kind, tag>& rhs)
{
    return std::tie(lhs.home, lhs.away) == std::tie(rhs.home, rhs.away);
}

template <typename TWinCase>
bool operator==(const TWinCase& lhs, const wincase_type& rhs)
{
    auto tagl = wincase_type::tag<TWinCase>::value;
    auto tagr = rhs.which();

    return tagl == tagr && lhs == rhs.get<TWinCase>();
}

template <typename TWinCase>
bool operator==(const wincase_type& lhs, const TWinCase& rhs)
{
    auto tagl = lhs.which();
    auto tagr = wincase_type::tag<TWinCase>::value;

    return tagl == tagr && lhs.get<std::decay_t<decltype(rhs)>>() == rhs;
}
// clang-format on
}
}
}
namespace std {

using namespace scorum::protocol::betting;

template <> struct less<wincase_type>
{
    bool operator()(const wincase_type& lhs, const wincase_type& rhs) const
    {
        auto tagl = lhs.which();
        auto tagr = rhs.which();

        return tagl < tagr
            || (!(tagr < tagl) && lhs.visit([&](const auto& l) { return l < rhs.get<std::decay_t<decltype(l)>>(); }));
    }

    template <typename TWinCase> bool operator()(const TWinCase& lhs, const wincase_type& rhs) const
    {
        return lhs < rhs;
    }

    template <typename TWinCase> bool operator()(const wincase_type& lhs, const TWinCase& rhs) const
    {
        return lhs < rhs;
    }
};

template <> struct equal_to<wincase_type>
{
    bool operator()(const wincase_type& lhs, const wincase_type& rhs) const
    {
        auto tagl = lhs.which();
        auto tagr = rhs.which();

        return tagl == tagr && lhs.visit([&](const auto& l) { return l == rhs.get<std::decay_t<decltype(l)>>(); });
    }

    template <typename TWinCase> bool operator()(const TWinCase& lhs, const wincase_type& rhs) const
    {
        return lhs == rhs;
    }

    template <typename TWinCase> bool operator()(const wincase_type& lhs, const TWinCase& rhs) const
    {
        return lhs == rhs;
    }
};
}