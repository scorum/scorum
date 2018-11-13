#pragma once
#include <boost/range/adaptor/transformed.hpp>

namespace scorum {
namespace utils {
template <template <typename...> class TContainer, typename TObject, typename... TOtherParams>
auto unwrap_ref_wrapper(const TContainer<std::reference_wrapper<const TObject>, TOtherParams...>& rng)
{
    return rng | boost::adaptors::transformed([](const auto& item) -> decltype(auto) { return item.get(); });
}

template <template <typename...> class TContainer, typename TObject, typename... TOtherParams>
auto unwrap_ref_wrapper(const TContainer<std::reference_wrapper<TObject>, TOtherParams...>& rng)
{
    return rng | boost::adaptors::transformed([](auto& item) -> decltype(auto) { return item.get(); });
}
}
}
