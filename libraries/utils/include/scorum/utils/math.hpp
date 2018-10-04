#pragma once
namespace scorum {
namespace utils {
template <typename T, typename U> T ceil(T value, T starting_point, U step)
{
    if (starting_point < value)
    {
        auto alignment_mismatch_from_left = (value - starting_point) % step;
        auto aligned_value = alignment_mismatch_from_left == 0 ? value : value + step - alignment_mismatch_from_left;
        return aligned_value;
    }
    else
    {
        auto alignment_mismatch_from_right = (starting_point - value) % step;
        auto aligned_value = value + alignment_mismatch_from_right;
        return aligned_value;
    }
}
}
}
