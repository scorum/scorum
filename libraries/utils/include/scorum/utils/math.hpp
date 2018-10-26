#pragma once
namespace scorum {
namespace utils {
/**
 * std::ceil analog which 'ceil' not to nearest greater integer, but to nearest greater integer starting from @par
 * starting_point with @par interval
 * @param value Value to ceil
 * @param starting_point Special value which satisfies condition: abs(<function_result> - starting_point)%interval == 0
 * @param interval Interval between points
 */
template <typename T, typename U> T ceil(T value, T starting_point, U interval)
{
    if (starting_point < value)
    {
        auto alignment_mismatch_from_left = (value - starting_point) % interval;
        auto aligned_value
            = alignment_mismatch_from_left == 0 ? value : value + interval - alignment_mismatch_from_left;
        return aligned_value;
    }
    else
    {
        auto alignment_mismatch_from_right = (starting_point - value) % interval;
        auto aligned_value = value + alignment_mismatch_from_right;
        return aligned_value;
    }
}
}
}
