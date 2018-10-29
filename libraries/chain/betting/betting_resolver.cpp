#include <scorum/chain/betting/betting_resolver.hpp>

#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/bet_objects.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/database/database_virtual_operations.hpp>

namespace scorum {
namespace chain {
betting_resolver::betting_resolver(account_service_i& account_svc,
                                   database_virtual_operations_emmiter_i& virt_op_emitter,
                                   dba::db_accessor<matched_bet_object>& matched_bet_dba,
                                   dba::db_accessor<game_object>& game_dba,
                                   dba::db_accessor<dynamic_global_property_object>& dprop_dba)
    : _account_svc(account_svc)
    , _virt_op_emitter(virt_op_emitter)
    , _matched_bet_dba(matched_bet_dba)
    , _game_dba(game_dba)
    , _dprop_dba(dprop_dba)
{
}

void betting_resolver::resolve_matched_bets(const game_id_type& game_id,
                                            const fc::flat_set<wincase_type>& results) const
{
    auto matched_bets = _matched_bet_dba.get_range_by<by_game_id_market>(game_id);
    const auto& game = _game_dba.get_by<by_id>(game_id);

    for (const matched_bet_object& bet : matched_bets)
    {
        auto fst_won = results.find(bet.bet1_data.wincase) != results.end();
        auto snd_won = results.find(bet.bet2_data.wincase) != results.end();

        if (fst_won)
        {
            auto income = bet.bet1_data.stake + bet.bet2_data.stake;
            _account_svc.increase_balance(bet.bet1_data.better, income);

            _virt_op_emitter.push_virtual_operation(bet_resolved_operation(
                game.uuid, bet.bet1_data.better, bet.bet1_data.uuid, income, bet_resolve_kind::win));
        }
        else if (snd_won)
        {
            auto income = bet.bet1_data.stake + bet.bet2_data.stake;
            _account_svc.increase_balance(bet.bet2_data.better, income);

            _virt_op_emitter.push_virtual_operation(bet_resolved_operation(
                game.uuid, bet.bet2_data.better, bet.bet2_data.uuid, income, bet_resolve_kind::win));
        }
        else
        {
            _account_svc.increase_balance(bet.bet1_data.better, bet.bet1_data.stake);
            _account_svc.increase_balance(bet.bet2_data.better, bet.bet2_data.stake);

            _virt_op_emitter.push_virtual_operation(bet_resolved_operation(
                game.uuid, bet.bet1_data.better, bet.bet1_data.uuid, bet.bet1_data.stake, bet_resolve_kind::draw));
            _virt_op_emitter.push_virtual_operation(bet_resolved_operation(
                game.uuid, bet.bet1_data.better, bet.bet1_data.uuid, bet.bet1_data.stake, bet_resolve_kind::draw));
        }

        _dprop_dba.update([&](dynamic_global_property_object& o) {
            o.betting_stats.matched_bets_volume -= bet.bet1_data.stake + bet.bet2_data.stake;
        });
    }

    _matched_bet_dba.remove_all(matched_bets);
}
}
}
