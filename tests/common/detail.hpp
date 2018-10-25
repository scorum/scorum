#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/name_generator.hpp>

#include <fc/io/raw.hpp>
#include <fc/crypto/hex.hpp>

namespace scorum {
namespace chain {
struct bet_data;
}
}

boost::uuids::uuid gen_uuid(const std::string& seed);

bool compare_bet_data(const scorum::chain::bet_data& lhs, const scorum::chain::bet_data& rhs);

namespace detail {

template <typename T> std::string to_hex(const T& o)
{
    return fc::to_hex(fc::raw::pack(o));
}

template <typename T> T from_hex(const std::string& bin_hex)
{
    char buffer[sizeof(T)];
    fc::from_hex(bin_hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<T>(buffer, sizeof(buffer));

    return obj;
}
}
