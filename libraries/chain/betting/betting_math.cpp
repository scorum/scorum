#include <scorum/chain/betting/betting_math.hpp>

namespace scorum {
namespace chain {
namespace betting {

using scorum::protocol::odds_fraction_type;

asset calculate_gain(const asset& bet_stake, const odds& bet_odds)
{
    FC_ASSERT(bet_stake.symbol() == SCORUM_SYMBOL, "Invalid sumbol for stake");
    auto result = bet_stake;
    result *= bet_odds;
    result -= bet_stake;
    return std::max(result, asset(1, result.symbol()));
}

asset calculate_matched_stake(const asset& bet1_stake,
                              const asset& bet2_stake,
                              const odds& bet1_odds,
                              const odds& bet2_odds)
{
    FC_ASSERT(bet1_stake.symbol() == SCORUM_SYMBOL && bet1_stake.symbol() == bet2_stake.symbol(),
              "Invalid sumbol for stake");
    FC_ASSERT((odds_fraction_type)bet1_odds == bet2_odds.inverted()
                  && (odds_fraction_type)bet2_odds == bet1_odds.inverted(),
              "Odds for bet 1 ans bet 2 must make 1 in summ of it's probability");

    auto r1 = bet1_stake * bet1_odds;
    auto r2 = bet2_stake * bet2_odds;

    if (r1 > r2)
    {
        auto result = r2;
        result *= odds_fraction_type(bet1_odds).coup();
        return std::max(result, asset(1, result.symbol())); // return positive value
    }
    else if (r1 < r2)
    {
        auto result = r1;
        result *= odds_fraction_type(bet2_odds).coup();
        return -std::max(result, asset(1, result.symbol())); // return negative value to check site
    }

    return bet1_stake;
}
}
}
}
