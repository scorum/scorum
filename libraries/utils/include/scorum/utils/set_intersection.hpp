namespace scorum {
namespace utils {
template <typename InputRng1, typename InputRng2, typename OutputIt, typename Comp, typename Selector>
OutputIt set_intersection(const InputRng1& rng1, const InputRng2& rng2, OutputIt out, Comp comp, Selector select)
{
    // see: https://en.cppreference.com/w/cpp/algorithm/set_intersection
    auto fst1 = std::begin(rng1);
    auto lst1 = std::end(rng1);
    auto fst2 = std::begin(rng2);
    auto lst2 = std::end(rng2);

    while (fst1 != lst1 && fst2 != lst2)
    {
        if (comp(*fst1, *fst2))
        {
            ++fst1;
        }
        else
        {
            if (!comp(*fst2, *fst1))
            {
                *out++ = select(*fst1, *fst2);
                ++fst1;
            }
            ++fst2;
        }
    }
    return out;
}
}
}
