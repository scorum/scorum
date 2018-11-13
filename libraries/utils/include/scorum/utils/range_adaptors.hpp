#pragma once
#include <cstddef>
#include <vector>
#include <cstdint>

namespace scorum {
namespace utils {

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
