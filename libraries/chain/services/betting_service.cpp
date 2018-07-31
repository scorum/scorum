#include <scorum/chain/services/betting_service.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>

namespace scorum {
namespace chain {

dbs_betting::dbs_betting(database& db)
    : _base_type(db)
    , _dgp_property(db.dynamic_global_property_service())
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

const bet_object& dbs_betting::create_bet(const account_name_type& better,
                                          const game_id_type game,
                                          const wincase_type& wincase,
                                          const std::string& odds_value,
                                          const asset& stake)
{
    return _bet.create([&](bet_object& obj) {
        obj.created = _dgp_property.head_block_time();
        obj.better = better;
        obj.game = game;
        obj.wincase = wincase;
        obj.value = odds::from_string(odds_value);
        obj.stake = stake;
        obj.rest_potential_return = obj.stake * obj.value;
    });
}

void dbs_betting::match(const bet_object& bet)
{
    FC_ASSERT(bet.rest_potential_return.amount > 0);

    using pending_bets_type = std::vector<pending_bet_service_i::object_cref_type>;

    pending_bets_type removing_pending_bets;

    _pending_bet.foreach_pending_bets(bet.game, [&](pending_bet_object& pbet) -> bool {

        const bet_object& pending_bet = _bet.get(pending_bet.bet);

        if (is_bets_matched(bet, pending_bet))
        {
            auto rest_potential_return = std::min(bet.rest_potential_return, pending_bet.rest_potential_return);
            if (rest_potential_return == pending_bet.rest_potential_return)
            {
                removing_pending_bets.emplace_back(std::cref(pending_bet));
            }

            _bet.update(bet, [&](bet_object& obj) { obj.rest_potential_return -= rest_potential_return; });
            _bet.update(pending_bet, [&](bet_object& obj) { obj.rest_potential_return -= rest_potential_return; });

            _matched_bet.create([&](matched_bet_object& obj) {
                obj.matched = _dgp_property.head_block_time();
                obj.bet1 = bet.id;
                obj.bet2 = pending_bet.id;
            });

            return bet.rest_potential_return.amount > 0;
        }

        return true;
    });

    for (const pending_bet_object& pending_bet : removing_pending_bets)
    {
        _pending_bet.remove(pending_bet);
    }
}

asset dbs_betting::get_matched_stake(const bet_object& bet)
{
    asset result = bet.stake * bet.value;
    result -= bet.rest_potential_return;
    return result;
}

bool dbs_betting::is_bets_matched(const bet_object& bet1, const bet_object& bet2)
{
    FC_ASSERT(bet1.game == bet2.game);
    return bet1.better != bet2.better && match_wincases(bet1.wincase, bet2.wincase)
        && bet1.value.inverted() == bet2.value;
}
}
}
