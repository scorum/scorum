#pragma once

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/proposal_operations.hpp>
#include <tuple>

namespace scorum {
namespace protocol {
namespace detail {

template <typename T> struct to_tuple;
template <typename... Ts> struct to_tuple<fc::static_variant<Ts...>>
{
    using type = std::tuple<Ts...>;
};

template <typename T> struct from_tuple;
template <typename... Ts> struct from_tuple<std::tuple<Ts...>>
{
    using type = fc::static_variant<Ts...>;
};

using operation_as_tuple = to_tuple<operation>::type;
using proposal_operation_as_tuple = to_tuple<proposal_operation>::type;

using unified_operation_as_tuple
    = decltype(std::tuple_cat(std::declval<operation_as_tuple>(), std::declval<proposal_operation_as_tuple>()));
}

using unified_operation = detail::from_tuple<detail::unified_operation_as_tuple>::type;
}
}