#include <scorum/chain/betting/betting_service.hpp>

#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/protocol/betting/wincase_comparison.hpp>

#include <scorum/chain/betting/betting_math.hpp>

#include <scorum/chain/schema/bet_objects.hpp>

namespace scorum {
namespace chain {
namespace betting {

betting_service::betting_service(data_service_factory_i& db)
    : _dgp_property_service(db.dynamic_global_property_service())
    , _betting_property_service(db.betting_property_service())
    , _matched_bet_svc(db.matched_bet_service())
    , _pending_bet_svc(db.pending_bet_service())
    , _bet_svc(db.bet_service())
    , _game_svc(db.game_service())
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

const bet_object& betting_service::create_bet(const account_name_type& better,
                                              const game_id_type game,
                                              const wincase_type& wincase,
                                              const odds& odds_value,
                                              const asset& stake)
{
    try
    {
        FC_ASSERT(stake.amount > 0);
        FC_ASSERT(stake.symbol() == SCORUM_SYMBOL);
        return _bet_svc.create([&](bet_object& obj) {
            obj.created = _dgp_property_service.head_block_time();
            obj.better = better;
            obj.game = game;
            obj.wincase = wincase;
            obj.odds_value = odds_value;
            obj.stake = stake;
            obj.rest_stake = stake;
        });
    }
    FC_CAPTURE_LOG_AND_RETHROW((better)(game)(wincase)(odds_value)(stake))
}

std::vector<std::reference_wrapper<const bet_object>>
betting_service::get_bets(const game_id_type& game, const std::vector<wincase_pair>& wincase_pairs) const
{
    // clang-format off
    struct less
    {
        bool operator()(const bet_object& l, const bet_object& r) const { return cmp(l.wincase, r.wincase); }
        bool operator()(const bet_object& b, const wincase_type& w) const { return cmp(b.wincase, w); }
        bool operator()(const wincase_type& w, const bet_object& b) const { return cmp(w, b.wincase); }
        std::less<wincase_type> cmp;
    };
    // clang-format on

    auto bets = _bet_svc.get_bets(game);
    boost::sort(bets, less{});

    fc::flat_set<wincase_type> wincases;
    wincases.reserve(wincase_pairs.size() * 2);

    for (const auto& pair : wincase_pairs)
    {
        wincases.emplace(pair.first);
        wincases.emplace(pair.second);
    }

    std::vector<std::reference_wrapper<const bet_object>> filtered_bets;
    boost::set_intersection(bets, wincases, std::back_inserter(filtered_bets), less{});

    return filtered_bets;
}

void betting_service::cancel_bets(const std::vector<std::reference_wrapper<const bet_object>>& bets) const
{
    for (const bet_object& bet : bets)
    {
        auto matched_bets_fst = _matched_bet_svc.get_bets_by_fst_better(bet.id);
        _matched_bet_svc.remove_all(matched_bets_fst);

        auto matched_bets_snd = _matched_bet_svc.get_bets_by_snd_better(bet.id);
        _matched_bet_svc.remove_all(matched_bets_snd);

        auto pending_bets = _pending_bet_svc.get_bets(bet.id);
        _pending_bet_svc.remove_all(pending_bets);
    }

    _bet_svc.remove_all(bets);
}

bool betting_service::is_bet_matched(const bet_object& bet) const
{
    return bet.rest_stake != bet.stake;
}

void betting_service::cancel_game(const game_id_type& game_id)
{
    auto matched_bets = _matched_bet_svc.get_bets(game_id);
    auto pending_bets = _pending_bet_svc.get_bets(game_id);
    auto bets = _bet_svc.get_bets(game_id);
    const auto& game = _game_svc.get_game(game_id._id);

    _pending_bet_svc.remove_all(pending_bets);
    _matched_bet_svc.remove_all(matched_bets);
    _bet_svc.remove_all(bets);
    _game_svc.remove(game);
}
}
}
}
