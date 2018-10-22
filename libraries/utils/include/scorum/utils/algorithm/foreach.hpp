namespace scorum {
namespace utils {
template <typename InputRng, typename Callback> void foreach (InputRng&& rng, Callback && callback)
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
