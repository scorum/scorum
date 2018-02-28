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

} // namespace utils
