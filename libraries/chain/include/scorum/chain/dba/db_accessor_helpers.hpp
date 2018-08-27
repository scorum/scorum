#pragma once
#include <boost/optional/optional.hpp>

namespace scorum {
namespace chain {
namespace dba {
struct param_placeholder
{
};
struct unbounded_placeholder
{
};
enum no_key
{
};

const param_placeholder _x = param_placeholder{};
const unbounded_placeholder unbounded = unbounded_placeholder{};

namespace detail {
enum class bound_kind
{
    lt,
    le,
    gt,
    ge,
    unbounded
};

template <typename T> struct bound
{
    boost::optional<const T&> value;
    bound_kind kind;

    bound(const T& value, bound_kind kind)
        : value(value)
        , kind(kind)
    {
    }

    bound(unbounded_placeholder)
        : kind(bound_kind::unbounded)
    {
    }
};
}

template <typename TValue> auto operator<(const param_placeholder&, const TValue& val)
{
    return detail::bound<TValue>{ val, detail::bound_kind::lt };
}

template <typename TValue> auto operator>(const TValue& val, const param_placeholder&)
{
    return detail::bound<TValue>{ val, detail::bound_kind::lt };
}

template <typename TValue> auto operator<=(const param_placeholder&, const TValue& val)
{
    return detail::bound<TValue>{ val, detail::bound_kind::le };
}

template <typename TValue> auto operator>=(const TValue& val, const param_placeholder&)
{
    return detail::bound<TValue>{ val, detail::bound_kind::le };
}

template <typename TValue> auto operator>(const param_placeholder&, const TValue& val)
{
    return detail::bound<TValue>{ val, detail::bound_kind::gt };
}

template <typename TValue> auto operator<(const TValue& val, const param_placeholder&)
{
    return detail::bound<TValue>{ val, detail::bound_kind::gt };
}

template <typename TValue> auto operator>=(const param_placeholder&, const TValue& val)
{
    return detail::bound<TValue>{ val, detail::bound_kind::ge };
}

template <typename TValue> auto operator<=(const TValue& val, const param_placeholder&)
{
    return detail::bound<TValue>{ val, detail::bound_kind::ge };
}
}
}
}
