#pragma once
#include <iterator>

namespace scorum {
namespace utils {
/**
 * Foreach loop which is able to modify range during iteration. Iterators after current iterator shouldn't be
 * invalidated
 */
template <typename InputRng, typename Callback> void foreach_mut(InputRng&& rng, Callback&& callback)
{
    for (auto it = std::begin(rng); it != std::end(rng);)
    {
        auto&& item = *it;
        ++it;
        callback(std::forward<decltype(item)>(item));
    }
}
}
}
