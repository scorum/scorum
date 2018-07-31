#pragma once
#include <fc/reflect/reflect.hpp>

namespace scorum {
namespace protocol {
namespace betting {

const int16_t threshold_factor = 1000;

struct threshold_type
{
    int16_t value;
};

inline bool operator<(const threshold_type& lhs, int16_t value)
{
    return lhs.value < threshold_factor * value;
}
inline bool operator<(int16_t value, const threshold_type& rhs)
{
    return threshold_factor * value < rhs.value;
}
inline bool operator>(const threshold_type& lhs, int16_t value)
{
    return lhs.value > threshold_factor * value;
}
inline bool operator>(int16_t value, const threshold_type& rhs)
{
    return threshold_factor * value > rhs.value;
}
inline bool operator<(const threshold_type& lhs, const threshold_type& rhs)
{
    return lhs.value < rhs.value;
}
inline bool operator>(const threshold_type& lhs, const threshold_type& rhs)
{
    return lhs.value > rhs.value;
}
}
}
}

FC_REFLECT(scorum::protocol::betting::threshold_type, (value))