#pragma once

#include <type_traits>

namespace scorum {

#define underlying(E) typename std::underlying_type<E>::type

template <typename E> constexpr auto to_underlying(E e) -> underlying(E)
{
    return static_cast<underlying(E)>(e);
}
}
