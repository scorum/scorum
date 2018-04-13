#pragma once

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/types.hpp>

#include <vector>

namespace scorum {
namespace rewards {

using scorum::protocol::share_type;
using scorum::protocol::curve_id;

using share_types = std::vector<share_type>;

share_type predict_payout(const uint128_t& recent_claims,
                          const share_type& rshares,
                          const share_type& reward_fund,
                          curve_id reward_curve,
                          const share_type& max_share);

uint128_t calculate_total_claims(const uint128_t& recent_claims,
                                 const time_point_sec& now,
                                 const time_point_sec& last_update,
                                 curve_id reward_curve,
                                 const share_types& vrshares);

share_type calculate_payout(const share_type& rshares,
                            const uint128_t& total_claims,
                            const share_type& reward_fund,
                            curve_id reward_curve,
                            const share_type& max_share);

share_type calculate_curations_payout(const share_type& payout);

share_type calculate_curation_payout(const share_type& curations_payout, uint64_t total_weight, uint64_t weight);
}
}
