#pragma once
#include <fc/reflect/reflect.hpp>

namespace scorum {
namespace protocol {
namespace betting {

struct threshold_type
{
    static constexpr int16_t factor = 1000;

    int16_t value;
};

inline bool operator<(const threshold_type& lhs, int16_t value)
{
    return lhs.value < threshold_type::factor * value;
}
inline bool operator<(int16_t value, const threshold_type& rhs)
{
    return threshold_type::factor * value < rhs.value;
}
inline bool operator>(const threshold_type& lhs, int16_t value)
{
    return lhs.value > threshold_type::factor * value;
}
inline bool operator>(int16_t value, const threshold_type& rhs)
{
    return threshold_type::factor * value > rhs.value;
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