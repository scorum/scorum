#pragma once

#include <scorum/protocol/types.hpp>

#include <fc/uint128.hpp>

namespace scorum {
namespace rewards {

using scorum::protocol::curve_id;
using fc::uint128_t;

uint128_t evaluate_reward_curve(const uint128_t& rshares, const curve_id& curve);

} // namespace rewards
} // namespace scorum
