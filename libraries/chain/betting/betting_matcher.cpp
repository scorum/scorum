#include <scorum/chain/betting/betting_matcher.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>

#include <scorum/protocol/betting/wincase_comparison.hpp>
#include <scorum/protocol/operations.hpp>

namespace scorum {
namespace chain {
namespace betting {

betting_matcher::betting_matcher(data_service_factory_i& db, database_virtual_operations_emmiter_i& virt_op_emitter)
    : _dgp_property(db.dynamic_global_property_service())
    , _betting_property(db.betting_property_service())
    , _bet_service(db.bet_service())
    , _pending_bet_service(db.pending_bet_service())
    , _matched_bet_service(db.matched_bet_service())
    , _virt_op_emitter(virt_op_emitter)
{
}

void betting_matcher::match(const bet_object& bet1)
{
    try
    {
        FC_ASSERT(is_need_matching(bet1));

        using pending_bets_type = std::vector<pending_bet_service_i::object_cref_type>;

        pending_bets_type removing_pending_bets;

        _pending_bet_service.foreach_pending_bets(bet1.game, [&](const pending_bet_object& pending_bet) {

            const bet_object& bet2 = _bet_service.get_bet(pending_bet.bet);

            if (!is_bets_matched(bet1, bet2))
                return true;

            auto matched = calculate_matched_stake(bet1.rest_stake, bet2.rest_stake, bet1.odds_value, bet2.odds_value);
            if (matched.bet1_matched.amount > 0 && matched.bet2_matched.amount > 0)
            {
                _bet_service.update(bet1, [&](bet_object& obj) { obj.rest_stake -= matched.bet1_matched; });
                _bet_service.update(bet2, [&](bet_object& obj) { obj.rest_stake -= matched.bet2_matched; });

                auto matched_bet = _matched_bet_service.create([&](matched_bet_object& obj) {
                    obj.when_matched = _dgp_property.head_block_time();
                    obj.bet1 = bet1.id;
                    obj.bet2 = bet2.id;
                    obj.matched_bet1_stake = matched.bet1_matched;
                    obj.matched_bet2_stake = matched.bet2_matched;
                });

                _virt_op_emitter.push_virtual_operation(protocol::bets_matched_operation(
                    bet1.better, bet2.better, matched.bet1_matched, matched.bet2_matched, matched_bet.id._id));
            }

            if (!is_need_matching(bet2))
            {
                removing_pending_bets.emplace_back(std::cref(pending_bet));
            }

            return is_need_matching(bet1);

        });

        if (is_need_matching(bet1))
        {
            _pending_bet_service.create([&](pending_bet_object& obj) {
                obj.game = bet1.game;
                obj.bet = bet1.id;
            });
        }

        for (const pending_bet_object& pending_bet : removing_pending_bets)
        {
            _pending_bet_service.remove(pending_bet);
        }
    }
    FC_CAPTURE_LOG_AND_RETHROW((bet1))
}

bool betting_matcher::is_bets_matched(const bet_object& bet1, const bet_object& bet2) const
{
    FC_ASSERT(bet1.game == bet2.game);
    return bet1.better != bet2.better && protocol::betting::match_wincases(bet1.wincase, bet2.wincase)
        && bet1.odds_value.inverted() == bet2.odds_value;
}

bool betting_matcher::is_need_matching(const bet_object& bet) const
{
    return bet.rest_stake.amount > SCORUM_MIN_BET_STAKE_FOR_MATCHING;
}
}
}
}
