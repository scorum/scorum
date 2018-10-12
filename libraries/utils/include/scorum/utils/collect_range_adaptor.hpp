#pragma once
#include <cstddef>
#include <vector>
#include <map>
#include <boost/container/flat_map.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace scorum {
namespace utils {
namespace adaptors {

template <template <typename...> class TContainer> struct collect
{
};

template <> struct collect<std::vector>
{
    collect(size_t reserve = 0)
        : reserve(reserve)
    {
    }

    size_t reserve;
};

template <> struct collect<boost::container::flat_map>
{
    collect(size_t reserve = 0)
        : reserve(reserve)
    {
    }

    size_t reserve;
};

namespace detail {
template <typename TRng, template <typename...> class TTargetContainer> struct collector
{
    template <typename URng> auto collect(URng&& rng, collect<TTargetContainer> c) const
    {
        return TTargetContainer<typename TRng::value_type>(std::begin(rng), std::end(rng));
    }
};

template <typename TRng> struct collector<TRng, std::map>
{
    template <typename URng> auto collect(URng&& rng, collect<std::map>) const
    {
        using key_type = typename TRng::value_type::first_type;
        using val_type = typename TRng::value_type::second_type;

        std::map<key_type, val_type> result(std::begin(rng), std::end(rng));

        return rng;
    }
};

template <typename TRng> struct collector<TRng, boost::container::flat_map>
{
    template <typename URng> auto collect(URng&& rng, collect<boost::container::flat_map> adaptor) const
    {
        using key_type = typename TRng::value_type::first_type;
        using val_type = typename TRng::value_type::second_type;

        boost::container::flat_map<key_type, val_type> result;
        result.reserve(adaptor.reserve);

        boost::copy(rng, std::inserter(result, std::end(result)));

        return result;
    }
};

template <typename TRng> struct collector<TRng, std::vector>
{
    template <typename URng> auto collect(URng&& rng, collect<std::vector> adaptor) const
    {
        std::vector<typename TRng::value_type> result;
        result.reserve(adaptor.reserve);
        result.assign(std::begin(rng), std::end(rng));

        return result;
    }
};
}

template <typename TRng, template <typename...> class TTargetContainer>
inline auto operator|(TRng&& rng, collect<TTargetContainer> adaptor)
{
    // TODO: Could be dramatically simplified with C++17 (class template agrument deduction)
    return detail::collector<TRng, TTargetContainer>().collect(std::forward<TRng>(rng), adaptor);
}
}
}
}
