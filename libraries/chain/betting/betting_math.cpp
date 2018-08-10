#include <scorum/chain/betting/betting_math.hpp>

namespace scorum {
namespace chain {
namespace betting {

using scorum::protocol::odds_fraction_type;

asset calculate_gain(const asset& bet_stake, const odds& bet_odds)
{
    FC_ASSERT(bet_stake.symbol() == SCORUM_SYMBOL, "Invalid symbol for stake");
    auto result = bet_stake;
    result *= bet_odds;
    result -= bet_stake;
    return result;
}

matched_stake_type
calculate_matched_stake(const asset& bet1_stake, const asset& bet2_stake, const odds& bet1_odds, const odds& bet2_odds)
{
    FC_ASSERT(bet1_stake.symbol() == SCORUM_SYMBOL && bet1_stake.symbol() == bet2_stake.symbol(),
              "Invalid symbol for stake");
    FC_ASSERT((odds_fraction_type)bet1_odds == bet2_odds.inverted()
                  && (odds_fraction_type)bet2_odds == bet1_odds.inverted(),
              "Odds for bet 1 ans bet 2 must make 1 in summ of it's probability");

    matched_stake_type result;

    auto r1 = bet1_stake * bet1_odds;
    auto r2 = bet2_stake * bet2_odds;

    auto p1 = r1 - bet1_stake;
    auto p2 = r2 - bet2_stake;

    FC_ASSERT(p1.amount > 0);
    FC_ASSERT(p2.amount > 0);

    if (r1 > r2)
    {
        result.bet1_matched = p2;
        result.bet2_matched = bet2_stake;
    }
    else if (r1 < r2)
    {
        result.bet2_matched = p1;
        result.bet1_matched = bet1_stake;
    }
    else
    {
        result.bet1_matched = bet1_stake;
        result.bet2_matched = bet2_stake;
    }

    return result;
}
}
}
}
