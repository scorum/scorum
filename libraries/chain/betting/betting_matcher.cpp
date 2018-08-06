#include <scorum/chain/betting/betting_matcher.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>

#include <scorum/protocol/betting/wincase_comparison.hpp>

namespace scorum {
namespace chain {
namespace betting {

betting_matcher::betting_matcher(data_service_factory_i& db)
    : _dgp_property(db.dynamic_global_property_service())
    , _betting_property(db.betting_property_service())
    , _bet_service(db.bet_service())
    , _pending_bet_service(db.pending_bet_service())
    , _matched_bet_service(db.matched_bet_service())
{
}

void betting_matcher::match(const bet_object& bet)
{
    try
    {
        FC_ASSERT(is_need_matching(bet));

        using pending_bets_type = std::vector<pending_bet_service_i::object_cref_type>;

        pending_bets_type removing_pending_bets;

        _pending_bet_service.foreach_pending_bets(bet.game, [&](const pending_bet_object& pending) -> bool {

            const bet_object& pending_bet = _bet_service.get_bet(pending.bet);

            if (is_bets_matched(bet, pending_bet))
            {
                auto matched
                    = calculate_matched_stake(bet.rest_stake, pending_bet.rest_stake, bet.value, pending_bet.value);
                if (matched.potential_bet1_result >= matched.potential_bet2_result)
                {
                    _bet_service.update(pending_bet, [&](bet_object& obj) {
                        obj.rest_stake.amount = 0;
                        obj.gain += matched.matched_stake;
                    });
                    _bet_service.update(bet, [&](bet_object& obj) {
                        obj.rest_stake -= matched.matched_stake;
                        obj.gain += pending_bet.stake;
                    });
                }
                else if (matched.potential_bet1_result < matched.potential_bet2_result)
                {
                    _bet_service.update(bet, [&](bet_object& obj) {
                        obj.rest_stake.amount = 0;
                        obj.gain += matched.matched_stake;
                    });
                    _bet_service.update(pending_bet, [&](bet_object& obj) {
                        obj.rest_stake -= matched.matched_stake;
                        obj.gain += bet.stake;
                    });
                }

                _matched_bet_service.create([&](matched_bet_object& obj) {
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
            _pending_bet_service.create([&](pending_bet_object& obj) {
                obj.game = bet.game;
                obj.bet = bet.id;
            });
        }

        for (const pending_bet_object& pending_bet : removing_pending_bets)
        {
            _pending_bet_service.remove(pending_bet);
        }
    }
    FC_CAPTURE_LOG_AND_RETHROW((bet))
}

bool betting_matcher::is_bets_matched(const bet_object& bet1, const bet_object& bet2) const
{
    FC_ASSERT(bet1.game == bet2.game);
    return bet1.better != bet2.better && protocol::betting::match_wincases(bet1.wincase, bet2.wincase)
        && bet1.value.inverted() == bet2.value;
}

bool betting_matcher::is_need_matching(const bet_object& bet) const
{
    return bet.gain.amount < bet.potential_gain.amount;
}
}
}
}
