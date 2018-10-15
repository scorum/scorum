#include <scorum/chain/betting/betting_matcher.hpp>

#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/game_object.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/chain/dba/db_accessor.hpp>

#include <scorum/protocol/betting/market.hpp>

#include <scorum/utils/range/unwrap_ref_wrapper_adaptor.hpp>

namespace scorum {
namespace chain {

betting_matcher::betting_matcher(data_service_factory_i& db,
                                 database_virtual_operations_emmiter_i& virt_op_emitter,
                                 betting_service_i& betting_svc,
                                 dba::db_accessor<game_object>& game_dba)
    : _betting_svc(betting_svc)
    , _dgp_property(db.dynamic_global_property_service())
    , _betting_property(db.betting_property_service())
    , _pending_bet_service(db.pending_bet_service())
    , _matched_bet_service(db.matched_bet_service())
    , _game_dba(game_dba)
    , _virt_op_emitter(virt_op_emitter)
{
}

void betting_matcher::match(const pending_bet_object& bet2)
{
    try
    {
        std::vector<std::reference_wrapper<const pending_bet_object>> pending_bets_to_cancel;

        _pending_bet_service.foreach_pending_bets(bet2.game, [&](const pending_bet_object& bet1) {
            if (!is_bets_matched(bet1, bet2))
                return true;

            auto matched
                = calculate_matched_stake(bet1.data.stake, bet2.data.stake, bet1.data.bet_odds, bet2.data.bet_odds);
            if (matched.bet1_matched.amount > 0 && matched.bet2_matched.amount > 0)
            {
                auto matched_bet = _matched_bet_service.create([&](matched_bet_object& obj) {
                    obj.bet1_data = bet1.data;
                    obj.bet2_data = bet2.data;
                    obj.bet1_data.stake = matched.bet1_matched;
                    obj.bet2_data.stake = matched.bet2_matched;
                    obj.market = create_market(bet1.data.wincase);
                    obj.game = bet1.game;
                    obj.created = _dgp_property.head_block_time();
                });

                _pending_bet_service.update(bet1, [&](pending_bet_object& o) { o.data.stake -= matched.bet1_matched; });
                _pending_bet_service.update(bet2, [&](pending_bet_object& o) { o.data.stake -= matched.bet2_matched; });

                _virt_op_emitter.push_virtual_operation(protocol::bets_matched_operation(
                    bet1.data.better, bet2.data.better, bet1.get_uuid(), bet2.get_uuid(), matched.bet1_matched,
                    matched.bet2_matched, matched_bet.id._id));
            }

            if (!can_be_matched(bet1.data.stake, bet1.data.bet_odds))
                pending_bets_to_cancel.emplace_back(bet1);
            if (!can_be_matched(bet2.data.stake, bet2.data.bet_odds))
                pending_bets_to_cancel.emplace_back(bet2);

            return can_be_matched(bet2.data.stake, bet2.data.bet_odds);
        });

        const auto& game = _game_dba.get_by<by_id>(bet2.game);

        _betting_svc.cancel_pending_bets(utils::unwrap_ref_wrapper(pending_bets_to_cancel), game.uuid);
    }
    FC_CAPTURE_LOG_AND_RETHROW((bet2))
}

bool betting_matcher::is_bets_matched(const pending_bet_object& bet1, const pending_bet_object& bet2) const
{
    FC_ASSERT(bet1.game == bet2.game);
    return bet1.data.better != bet2.data.better && match_wincases(bet1.data.wincase, bet2.data.wincase)
        && bet1.data.bet_odds.inverted() == bet2.data.bet_odds;
}

bool betting_matcher::can_be_matched(const asset& stake, const odds& bet_odds) const
{
    return stake * bet_odds > stake;
}
} // namespace chain
} // namespace scorum
