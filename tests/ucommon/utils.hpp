#pragma once

#include <string>
#include <vector>

#include <fc/io/json.hpp>
#include <fc/crypto/hex.hpp>

namespace utils {

template <typename T> std::string to_hex(const T& obj)
{
    std::stringstream ss;
    fc::raw::pack(ss, obj);
    std::string buf = ss.str();

    return fc::to_hex(buf.c_str(), buf.size());
}

template <typename T> T from_hex(const std::string& bin_hex)
{
    char buffer[sizeof(T)];
    fc::from_hex(bin_hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<T>(buffer, sizeof(buffer));

    return obj;
}

} // namespace utils
