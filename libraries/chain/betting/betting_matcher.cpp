#include <scorum/chain/betting/betting_matcher.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/protocol/betting/market.hpp>

namespace scorum {
namespace chain {

betting_matcher::betting_matcher(data_service_factory_i& db,
                                 database_virtual_operations_emmiter_i& virt_op_emitter,
                                 betting_service_i& betting_svc)
    : _betting_svc(betting_svc)
    , _dgp_property(db.dynamic_global_property_service())
    , _betting_property(db.betting_property_service())
    , _pending_bet_service(db.pending_bet_service())
    , _matched_bet_service(db.matched_bet_service())
    , _virt_op_emitter(virt_op_emitter)
{
}

void betting_matcher::match(const pending_bet_object& bet2)
{
    try
    {
        betting_service_i::pending_bet_crefs_type pending_bets_to_cancel;

        _pending_bet_service.foreach_pending_bets(bet2.game, [&](const pending_bet_object& bet1) {

            if (!is_bets_matched(bet1, bet2))
                return true;

            auto matched = calculate_matched_stake(bet1.stake, bet2.stake, bet1.odds_value, bet2.odds_value);
            if (matched.bet1_matched.amount > 0 && matched.bet2_matched.amount > 0)
            {
                auto matched_bet = _matched_bet_service.create([&](matched_bet_object& obj) {
                    obj.wincase1 = bet1.wincase;
                    obj.wincase2 = bet2.wincase;
                    obj.better1 = bet1.better;
                    obj.better2 = bet2.better;
                    obj.market = create_market(bet1.wincase);
                    obj.game = bet1.game;
                    obj.created = _dgp_property.head_block_time();
                    obj.stake1 = matched.bet1_matched;
                    obj.stake2 = matched.bet2_matched;
                });

                _pending_bet_service.update(bet1, [&](pending_bet_object& o) { o.stake -= matched.bet1_matched; });
                _pending_bet_service.update(bet2, [&](pending_bet_object& o) { o.stake -= matched.bet2_matched; });

                _virt_op_emitter.push_virtual_operation(protocol::bets_matched_operation(
                    bet1.better, bet2.better, matched.bet1_matched, matched.bet2_matched, matched_bet.id._id));
            }

            if (!can_be_matched(bet1.stake, bet1.odds_value))
                pending_bets_to_cancel.emplace_back(bet1);
            if (!can_be_matched(bet2.stake, bet2.odds_value))
                pending_bets_to_cancel.emplace_back(bet2);

            return can_be_matched(bet2.stake, bet2.odds_value);
        });

        _betting_svc.cancel_pending_bets(pending_bets_to_cancel);
    }
    FC_CAPTURE_LOG_AND_RETHROW((bet2))
}

bool betting_matcher::is_bets_matched(const pending_bet_object& bet1, const pending_bet_object& bet2) const
{
    FC_ASSERT(bet1.game == bet2.game);
    return bet1.better != bet2.better && match_wincases(bet1.wincase, bet2.wincase)
        && bet1.odds_value.inverted() == bet2.odds_value;
}

bool betting_matcher::can_be_matched(const asset& stake, const odds& bet_odds) const
{
    return stake * bet_odds > stake;
}
}
}
