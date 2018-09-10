#pragma once
#include <fc/static_variant.hpp>

namespace scorum {
namespace utils {
template <typename... TArgs>
bool variant_less(const fc::static_variant<TArgs...>& lhs, const fc::static_variant<TArgs...>& rhs)
{
    return lhs.which() < rhs.which() || (!(rhs.which() < lhs.which()) && lhs.visit([&](const auto& l) {
               return l < rhs.template get<std::decay_t<decltype(l)>>();
           }));
}

template <typename... TArgs>
bool variant_eq(const fc::static_variant<TArgs...>& lhs, const fc::static_variant<TArgs...>& rhs)
{
    return !variant_less(lhs, rhs) && !variant_less(rhs, lhs);
}
}
}
