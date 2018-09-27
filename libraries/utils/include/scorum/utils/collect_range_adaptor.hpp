#pragma once
#include <cstddef>
#include <vector>
#include <cstdint>
#include <boost/range/algorithm/copy.hpp>

namespace scorum {
namespace utils {
namespace adaptors {
namespace traits {
template <class...> using void_t = void;
template <typename T, typename = void> struct has_push_back : std::false_type
{
};
template <typename T>
struct has_push_back<T, void_t<decltype(std::declval<T>().push_back(*std::declval<T>().begin()))>> : std::true_type
{
};

template <typename T, typename = void> struct has_reserve : std::false_type
{
};
template <typename T> struct has_reserve<T, void_t<decltype(std::declval<T>().reserve(size_t(0)))>> : std::true_type
{
};
}
namespace detail {

template <typename T> auto get_inserter(std::enable_if_t<traits::has_push_back<T>::value, T&> container)
{
    return std::back_inserter(container);
}
template <typename T> auto get_inserter(std::enable_if_t<!traits::has_push_back<T>::value, T&> container)
{
    return std::inserter(container, container.end());
}

template <typename T> void reserve(std::enable_if_t<traits::has_reserve<T>::value, T&> container, size_t n)
{
    container.reserve(n);
}
template <typename T> void reserve(std::enable_if_t<!traits::has_reserve<T>::value, T&> container, size_t)
{
}
}

template <typename TContainer> struct collect
{
    collect(size_t n = 0)
        : n(n)
    {
    }

    size_t n;
};

template <typename TRng, typename TTargetContainer>
inline auto operator|(TRng&& rng, const adaptors::collect<TTargetContainer>& adaptor)
{
    TTargetContainer container;
    detail::reserve<TTargetContainer>(container, adaptor.n);

    boost::copy(rng, detail::get_inserter<TTargetContainer>(container));

    return container;
}
}
}
}
