#include <scorum/chain/betting/betting_service.hpp>

#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm/find_if.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/protocol/betting/market.hpp>
#include <scorum/chain/betting/betting_math.hpp>
#include <scorum/chain/schema/bet_objects.hpp>

namespace scorum {
namespace chain {

betting_service::betting_service(data_service_factory_i& db)
    : _dgp_property_service(db.dynamic_global_property_service())
    , _betting_property_service(db.betting_property_service())
    , _matched_bet_svc(db.matched_bet_service())
    , _pending_bet_svc(db.pending_bet_service())
    , _game_svc(db.game_service())
    , _account_svc(db.account_service())
{
}

bool betting_service::is_betting_moderator(const account_name_type& account_name) const
{
    try
    {
        return _betting_property_service.get().moderator == account_name;
    }
    FC_CAPTURE_LOG_AND_RETHROW((account_name))
}

void betting_service::cancel_game(const game_id_type& game_id)
{
    // TODO: will be refactored using db_accessors & bidir_ranges
    auto matched_bets = _matched_bet_svc.get_bets(game_id);
    FC_ASSERT(matched_bets.empty(), "Cannot cancel game which has associated bets");

    auto pending_bets = _matched_bet_svc.get_bets(game_id);
    FC_ASSERT(pending_bets.empty(), "Cannot cancel game which has associated bets");

    const auto& game = _game_svc.get_game(game_id._id);
    _game_svc.remove(game);
}

void betting_service::cancel_bets(const game_id_type& game_id)
{
    cancel_pending_bets(game_id);
    cancel_matched_bets(game_id);
}

void betting_service::cancel_bets(const game_id_type& game_id, fc::time_point_sec created_from)
{
    auto matched_bets = _matched_bet_svc.get_bets(game_id, created_from);
    auto pending_bets = _pending_bet_svc.get_bets(game_id, created_from);

    cancel_pending_bets(pending_bets);

    for (const matched_bet_object& matched_bet : matched_bets)
    {
        return_or_restore_bet(matched_bet.bet1_data, matched_bet.game, created_from);
        return_or_restore_bet(matched_bet.bet2_data, matched_bet.game, created_from);
    }

    _matched_bet_svc.remove_all(matched_bets);
}

void betting_service::cancel_bets(const game_id_type& game_id, const fc::flat_set<market_type>& cancelled_markets)
{
    // clang-format off
    struct less
    {
        bool operator()(const pending_bet_object& b, const market_type& m) const { return b.market < m; }
        bool operator()(const market_type& m, const pending_bet_object& b) const { return m < b.market; }
        bool operator()(const matched_bet_object& b, const market_type& m) const { return b.market < m; }
        bool operator()(const market_type& m, const matched_bet_object& b) const { return m < b.market; }
    };
    // clang-format on

    auto pending_bets = _pending_bet_svc.get_bets(game_id);

    std::vector<std::reference_wrapper<const pending_bet_object>> filtered_pending_bets;
    boost::set_intersection(pending_bets, cancelled_markets, std::back_inserter(filtered_pending_bets), less{});

    cancel_pending_bets(filtered_pending_bets);

    auto matched_bets = _matched_bet_svc.get_bets(game_id);

    std::vector<std::reference_wrapper<const matched_bet_object>> filtered_matched_bets;
    boost::set_intersection(matched_bets, cancelled_markets, std::back_inserter(filtered_matched_bets), less{});

    cancel_matched_bets(filtered_matched_bets);
}

void betting_service::cancel_pending_bet(pending_bet_id_type id)
{
    const auto& pending_bet = _pending_bet_svc.get_pending_bet(id);
    _account_svc.increase_balance(pending_bet.data.better, pending_bet.data.stake);

    _pending_bet_svc.remove(pending_bet);
}

void betting_service::cancel_pending_bets(const game_id_type& game_id)
{
    auto pending_bets = _pending_bet_svc.get_bets(game_id);
    cancel_pending_bets(pending_bets);
}

void betting_service::cancel_pending_bets(const game_id_type& game_id, pending_bet_kind kind)
{
    auto pending_bets = _pending_bet_svc.get_bets(game_id, kind);
    cancel_pending_bets(pending_bets);
}

void betting_service::cancel_pending_bets(const pending_bet_crefs_type& pending_bets)
{
    for (const pending_bet_object& pending_bet : pending_bets)
    {
        _account_svc.increase_balance(pending_bet.data.better, pending_bet.data.stake);
    }

    _pending_bet_svc.remove_all(pending_bets);
}

void betting_service::cancel_matched_bets(const game_id_type& game_id)
{
    auto matched_bets = _matched_bet_svc.get_bets(game_id);
    cancel_matched_bets(matched_bets);
}

void betting_service::cancel_matched_bets(const matched_bet_crefs_type& matched_bets)
{
    for (const matched_bet_object& matched_bet : matched_bets)
    {
        _account_svc.increase_balance(matched_bet.bet1_data.better, matched_bet.bet1_data.stake);
        _account_svc.increase_balance(matched_bet.bet2_data.better, matched_bet.bet2_data.stake);
    }

    _matched_bet_svc.remove_all(matched_bets);
}

void betting_service::return_or_restore_bet(const bet_data& bet, game_id_type game_id, fc::time_point_sec threshold)
{
    if (bet.created >= threshold)
    {
        _account_svc.increase_balance(bet.better, bet.stake);
    }
    else
    {
        auto bets = _pending_bet_svc.get_bets(game_id, bet.better);
        // clang-format off
        auto found_it = boost::find_if(bets, [&](const pending_bet_object& o) {
            return o.data.created == bet.created
                && o.data.bet_odds == bet.bet_odds
                && o.data.kind == bet.kind
                && !(o.data.wincase < bet.wincase)
                && !(bet.wincase < o.data.wincase);
        });
        // clang-format on

        if (found_it != bets.end())
        {
            _pending_bet_svc.update(*found_it, [&](pending_bet_object& o) { o.data.stake += bet.stake; });
        }
        else
        {
            _pending_bet_svc.create([&](pending_bet_object& o) {
                o.game = game_id;
                o.market = create_market(bet.wincase);
                o.data = bet;
            });
        }
    }
}
}
}
