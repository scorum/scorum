#include <scorum/chain/services/betting_service.hpp>

#include <scorum/chain/services/betting_property.hpp>

namespace scorum {
namespace chain {
dbs_betting::dbs_betting(database& db)
    : _base_type(db)
    , _betting_property(db.betting_property_service())
{
}

bool dbs_betting::is_betting_moderator(const account_name_type& account_name) const
{
    try
    {
        return _betting_property.get().moderator == account_name;
    }
    FC_CAPTURE_LOG_AND_RETHROW((account_name))
}

auto dbs_betting::get_matched_stake(const asset& bet1_stake,
                                    const asset& bet2_stake,
                                    const odds& bet1_odds,
                                    const odds& bet2_odds)
{
    FC_ASSERT(bet1_stake.symbol() == SCORUM_SYMBOL && bet1_stake.symbol() == bet2_stake.symbol(),
              "Invalid sumbol for stake");
    FC_ASSERT((odds_fraction_type)bet1_odds == bet1_odds.inverted()
                  && bet2_odds.inverted() == (odds_fraction_type)bet1_odds,
              "Odds for bet 1 ans bet 2 must make 1 in summ");

    auto r1 = bet1_stake * bet1_odds;
    auto r2 = bet2_stake * bet2_odds;

    auto result = r1;
    result -= r2 * (bet2_odds / bet1_odds);
    return result;
}
}
}
