#include <scorum/chain/services/betting_service.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>

namespace scorum {
namespace chain {

asset calculate_gain(const asset& bet_stake, const odds& bet_odds)
{
    auto result = bet_stake;
    result *= bet_odds;
    result -= bet_stake;
    return result;
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
        return result; // return positive value
    }
    else if (r1 < r2)
    {
        auto result = r1;
        result *= odds_fraction_type(bet2_odds).coup();
        return -result; // return negative value to check side
    }

    return bet1_stake;
}

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
    try
    {
        return _bet.create([&](bet_object& obj) {
            obj.created = _dgp_property.head_block_time();
            obj.better = better;
            obj.game = game;
            obj.wincase = wincase;
            obj.value = odds::from_string(odds_value);
            obj.stake = stake;
            obj.rest_stake = stake;
            obj.potential_gain = calculate_gain(obj.stake, obj.value);
        });
    }
    FC_CAPTURE_LOG_AND_RETHROW((better)(game)(odds_value)(stake)) // TODO: wincase reflection
}

void dbs_betting::match(const bet_object& bet)
{
    try
    {

        FC_ASSERT(is_need_matching(bet));

        using pending_bets_type = std::vector<pending_bet_service_i::object_cref_type>;

        pending_bets_type removing_pending_bets;

        _pending_bet.foreach_pending_bets(bet.game, [&](const pending_bet_object& pending) -> bool {

            const bet_object& pending_bet = _bet.get_bet(pending.bet);

            if (is_bets_matched(bet, pending_bet))
            {
                auto matched_stake
                    = calculate_matched_stake(bet.rest_stake, pending_bet.rest_stake, bet.value, pending_bet.value);
                if (matched_stake.amount >= 0)
                {
                    _bet.update(pending_bet, [&](bet_object& obj) {
                        obj.rest_stake.amount = 0;
                        obj.gain += matched_stake.amount;
                    });
                    _bet.update(bet, [&](bet_object& obj) {
                        obj.rest_stake.amount -= matched_stake.amount;
                        obj.gain += pending_bet.stake;
                    });
                }
                else if (matched_stake.amount < 0)
                {
                    _bet.update(bet, [&](bet_object& obj) {
                        obj.rest_stake.amount = 0;
                        obj.gain -= matched_stake.amount;
                    });
                    _bet.update(pending_bet, [&](bet_object& obj) {
                        obj.rest_stake.amount += matched_stake.amount;
                        obj.gain += bet.stake;
                    });
                }

                _matched_bet.create([&](matched_bet_object& obj) {
                    obj.matched = _dgp_property.head_block_time();
                    obj.bet1 = bet.id;
                    obj.bet2 = pending_bet.id;
                });

                if (!is_need_matching(pending_bet))
                {
                    removing_pending_bets.emplace_back(std::cref(pending));
                }

                return is_need_matching(bet);
            }

            return true;
        });

        if (is_need_matching(bet))
        {
            _pending_bet.create([&](pending_bet_object& obj) {
                obj.game = bet.game;
                obj.bet = bet.id;
            });
        }

        for (const pending_bet_object& pending_bet : removing_pending_bets)
        {
            _pending_bet.remove(pending_bet);
        }
    }
    FC_CAPTURE_LOG_AND_RETHROW(()) // TODO: bet.wincase reflection
}

asset dbs_betting::get_gain(const bet_object& bet) const
{
    return bet.gain;
}

asset dbs_betting::get_rest(const bet_object& bet) const
{
    return bet.rest_stake;
}

bool dbs_betting::is_bets_matched(const bet_object& bet1, const bet_object& bet2) const
{
    FC_ASSERT(bet1.game == bet2.game);
    return bet1.better != bet2.better && match_wincases(bet1.wincase, bet2.wincase)
        && bet1.value.inverted() == bet2.value;
}

bool dbs_betting::is_need_matching(const bet_object& bet) const
{
    return bet.gain.amount < bet.potential_gain.amount;
}
}
}
