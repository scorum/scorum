#include <scorum/chain/evaluators/post_game_results_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/protocol/betting/market.hpp>

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>

namespace scorum {
namespace chain {
post_game_results_evaluator::post_game_results_evaluator(data_service_factory_i& services,
                                                         betting_service_i& betting_service,
                                                         database_virtual_operations_emmiter_i& virt_op_emitter)
    : evaluator_impl<data_service_factory_i, post_game_results_evaluator>(services)
    , _account_service(services.account_service())
    , _betting_service(betting_service)
    , _game_service(services.game_service())
    , _dprops_service(services.dynamic_global_property_service())
    , _virt_op_emitter(virt_op_emitter)
{
}

void post_game_results_evaluator::do_apply(const operation_type& op)
{
    _account_service.check_account_existence(op.moderator);
    FC_ASSERT(_betting_service.is_betting_moderator(op.moderator), "User ${u} isn't a betting moderator",
              ("u", op.moderator));
    FC_ASSERT(_game_service.is_exists(op.uuid), "Game with uuid '${g}' doesn't exist", ("g", op.uuid));

    const auto& game = _game_service.get_game(op.uuid);
    FC_ASSERT(game.status != game_status::created, "The game is not started yet");
    FC_ASSERT(game.bets_resolve_time > _dprops_service.head_block_time(),
              "Unable to post game results after bets were resolved");

    const fc::flat_set<wincase_type> wincases(op.wincases.begin(), op.wincases.end());

    std::set<market_kind> actual_markets = get_markets_kind(game.markets);

    for (auto wincase : wincases)
    {
        FC_ASSERT(actual_markets.find(get_market_kind(wincase)) != actual_markets.end(),
                  "Wincase '${w}' dont belongs to game markets", ("w", wincase));
    }

    validate_all_winners_present(game.markets, wincases);
    validate_opposite_winners_absent(wincases);

    _betting_service.cancel_pending_bets(game.uuid);

    auto old_status = game.status;
    _game_service.finish(game, wincases);

    if (old_status == game_status::started)
        _virt_op_emitter.push_virtual_operation(
            game_status_changed_operation(game.uuid, old_status, game_status::finished));
}

void post_game_results_evaluator::validate_all_winners_present(const fc::shared_flat_set<market_type>& markets,
                                                               const fc::flat_set<wincase_type>& winners) const
{
    using namespace boost::adaptors;
    using namespace boost::algorithm;

    for (const auto& market : markets | filtered([](const auto& m) { return !has_trd_state(m); }))
    {
        market.visit([&](const auto& m) {
            auto pair = create_wincases(m);
            auto exists = any_of(winners, [&](const auto& w) { return pair.first == w || pair.second == w; });

            FC_ASSERT(exists, "Wincase winners list do not contain neither '${1}' nor '${2}'",
                      ("1", pair.first)("2", pair.second));
        });
    }
}

void post_game_results_evaluator::validate_opposite_winners_absent(const fc::flat_set<wincase_type>& winners) const
{
    std::vector<wincase_type> opposite;
    opposite.reserve(winners.size());

    boost::transform(winners, std::back_inserter(opposite), [](const auto& w) { return create_opposite(w); });

    std::vector<wincase_type> intersection;
    intersection.reserve(winners.size());

    boost::set_intersection(winners, opposite, std::back_inserter(intersection));

    FC_ASSERT(intersection.empty(), "You've provided opposite wincases from same market as winners");
}
}
}
