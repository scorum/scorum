#pragma once

#include <scorum/protocol/odds.hpp>
#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::odds;

asset calculate_gain(const asset& bet_stake, const odds& bet_odds);

struct matched_stake_type
{
    asset bet1_matched = asset(0, SCORUM_SYMBOL);
    asset bet2_matched = asset(0, SCORUM_SYMBOL);
};

matched_stake_type
calculate_matched_stake(const asset& bet1_stake, const asset& bet2_stake, const odds& bet1_odds, const odds& bet2_odds);
}
}
