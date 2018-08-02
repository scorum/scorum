#pragma once
#include <cstddef>
#include <vector>
#include <cstdint>

namespace scorum {
namespace utils {
template <typename T> std::vector<T> take_n(const std::vector<T>& vec, int64_t n)
{
    auto to_take = std::min(n, (int64_t)vec.size());
    to_take = std::max(to_take, 0l);

    std::vector<T> rng;
    rng.reserve(to_take);

    for (int i = 0; i < to_take; ++i)
        rng.push_back(vec[i]);

    return rng;
}

template <typename TCollection, typename TSelector> auto flatten(const TCollection& src, TSelector&& selector)
{
    using inner_collection_type = decltype(selector(*src.begin()));
    using value_type = typename std::decay_t<inner_collection_type>::value_type;

    std::vector<value_type> result;

    for (const auto& obj : src)
    {
        const auto& inner_collection = selector(obj);
        for (const auto& item : inner_collection)
        {
            result.emplace_back(item);
        }
    }

    return result;
}
}
}