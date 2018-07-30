#include <scorum/chain/services/betting_service.hpp>

#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>

namespace scorum {
namespace chain {

asset get_matched_stake(const asset& bet1_stake, const asset& bet2_stake, const odds& bet1_odds, const odds& bet2_odds)
{
    FC_ASSERT(bet1_stake.symbol() == SCORUM_SYMBOL && bet1_stake.symbol() == bet2_stake.symbol(),
              "Invalid sumbol for stake");
    FC_ASSERT((odds_fraction_type)bet1_odds == bet1_odds.inverted()
                  && bet2_odds.inverted() == (odds_fraction_type)bet1_odds,
              "Odds for bet 1 ans bet 2 must make 1 in summ");

    auto r1 = bet1_stake * bet1_odds;
    auto r2 = bet2_stake * bet2_odds;

    auto result = r2;
    result *= bet2_odds;
    result *= odds_fraction_type(bet1_odds).coup();
    return r1 - result;
}

dbs_betting::dbs_betting(database& db)
    : _base_type(db)
    , _betting_property(db.betting_property_service())
    , _bet(db.bet_service())
    , _pending_bet(db.pending_bet_service())
    , _matched_bet(db.matched_bet_service())
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

void dbs_betting::match(const bet_object& bet)
{
    _pending_bet.foreach_pending_bets(bet.game, [&](pending_bet_object& pbet) -> bool {

        const bet_object& pending_bet = _bet.get(pbet.bet);

        // TODO: need matching for wincases
        if (bet.wincase == pending_bet.wincase && bet.value.inverted() == pending_bet.value)
        {
            // TODO

            return false;
        }

        return true;
    });

    // TODO
}
}
}
