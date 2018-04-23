#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::curve_id;

class reward_fund_object : public object<reward_fund_object_type, reward_fund_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(reward_fund_object)

    id_type id;

    asset activity_reward_balance_scr = asset(0, SCORUM_SYMBOL);
    fc::uint128_t recent_claims = 0;
    time_point_sec last_update;
    curve_id author_reward_curve;
    curve_id curation_reward_curve;
};

struct by_name;
typedef shared_multi_index_container<reward_fund_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<reward_fund_object,
                                                                      reward_fund_id_type,
                                                                      &reward_fund_object::id>>>>
    reward_fund_index;

} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::reward_fund_object,
           (id)(activity_reward_balance_scr)(recent_claims)(last_update)(author_reward_curve)(curation_reward_curve))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::reward_fund_object, scorum::chain::reward_fund_index)
