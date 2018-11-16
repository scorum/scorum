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

void update(fc::flat_map<uuid_type, bet_resolved_operation>& results,
            const bet_data& bet,
            asset income,
            uuid_type game_uuid,
            bet_resolve_kind kind)
{
    if (results.count(bet.uuid) == 0)
    {
        results.insert(std::make_pair(bet.uuid, bet_resolved_operation(game_uuid, bet.better, bet.uuid, income, kind)));
    }
    else
    {
        results[bet.uuid].income += income;
    }
}

struct resolver_results
{
    explicit resolver_results(uuid_type game_uuid)
        : _game_uuid(game_uuid)
    {
    }

    void post(const bet_data& bet, asset income, bet_resolve_kind kind)
    {
        if (_results.count(bet.uuid) == 0)
        {
            _results.insert(
                std::make_pair(bet.uuid, bet_resolved_operation(_game_uuid, bet.better, bet.uuid, income, kind)));
        }
        else
        {
            _results[bet.uuid].income += income;
        }
    }

    void apply(database_virtual_operations_emmiter_i& _emitter,
               account_service_i& _account_svc,
               dba::db_accessor<dynamic_global_property_object>& _dprop_dba)
    {
        for (auto& bet : _results)
        {
            _emitter.push_virtual_operation(bet.second);

            _account_svc.increase_balance(bet.second.better, bet.second.income);

            _dprop_dba.update([&](dynamic_global_property_object& o) { //
                o.betting_stats.matched_bets_volume -= bet.second.income;
            });
        }
    }

private:
    uuid_type _game_uuid;

    fc::flat_map<uuid_type, bet_resolved_operation> _results;
};

void betting_resolver::resolve_matched_bets(uuid_type game_uuid, const fc::flat_set<wincase_type>& results) const
{
    auto matched_bets = _matched_bet_dba.get_range_by<by_game_uuid_market>(game_uuid);

    resolver_results resolver(game_uuid);

    for (const matched_bet_object& bet : matched_bets)
    {
        auto fst_won = results.find(bet.bet1_data.wincase) != results.end();
        auto snd_won = results.find(bet.bet2_data.wincase) != results.end();

        auto income = bet.bet1_data.stake + bet.bet2_data.stake;

        if (fst_won)
        {
            resolver.post(bet.bet1_data, income, bet_resolve_kind::win);
        }
        else if (snd_won)
        {
            resolver.post(bet.bet2_data, income, bet_resolve_kind::win);
        }
        else
        {
            resolver.post(bet.bet1_data, bet.bet1_data.stake, bet_resolve_kind::draw);
            resolver.post(bet.bet2_data, bet.bet2_data.stake, bet_resolve_kind::draw);
        }
    }

    resolver.apply(_virt_op_emitter, _account_svc, _dprop_dba);
    _matched_bet_dba.remove_all(matched_bets);
}
}
}
