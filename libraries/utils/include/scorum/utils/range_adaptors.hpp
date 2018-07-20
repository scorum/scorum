#pragma once
#include <cstddef>
#include <vector>

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
}
}