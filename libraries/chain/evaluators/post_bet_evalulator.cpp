#include <scorum/chain/evaluators/post_bet_evalulator.hpp>

#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/dba/db_accessor.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/betting/betting_matcher.hpp>

#include <scorum/protocol/betting/invariants_validation.hpp>
#include <scorum/protocol/betting/market.hpp>

#include <scorum/utils/range/unwrap_ref_wrapper_adaptor.hpp>

#include <boost/range/algorithm/transform.hpp>

namespace scorum {
namespace chain {
post_bet_evaluator::post_bet_evaluator(data_service_factory_i& services,
                                       betting_matcher_i& betting_matcher,
                                       betting_service_i& betting_service,
                                       dba::db_accessor<game_object>& game_dba,
                                       dba::db_accessor<account_object>& account_dba,
                                       dba::db_accessor<bet_uuid_history_object>& uuid_hist_dba)
    : evaluator_impl<data_service_factory_i, post_bet_evaluator>(services)
    , _betting_matcher(betting_matcher)
    , _betting_svc(betting_service)
    , _game_dba(game_dba)
    , _account_dba(account_dba)
    , _uuid_hist_dba(uuid_hist_dba)
{
}

void post_bet_evaluator::do_apply(const operation_type& op)
{
    FC_ASSERT(_game_dba.is_exists_by<by_uuid>(op.game_uuid), "Game with uuid ${1} doesn't exist", ("1", op.game_uuid));
    FC_ASSERT(!_uuid_hist_dba.is_exists_by<by_uuid>(op.uuid), "Bet with uuid ${1} already exists", ("1", op.uuid));

    auto game_obj = _game_dba.get_by<by_uuid>(op.game_uuid);

    validate_if_wincase_in_game(game_obj.game, op.wincase);

    std::set<market_kind> actual_markets = get_markets_kind(game_obj.markets);

    FC_ASSERT(actual_markets.find(get_market_kind(op.wincase)) != actual_markets.end(),
              "Wincase '${w}' dont belongs to game markets", ("w", op.wincase));

    FC_ASSERT(game_obj.status != game_status::finished, "Cannot post bet for game that is finished");
    FC_ASSERT(game_obj.status == game_status::created || op.live, "Cannot create non-live bet after game was started");

    FC_ASSERT(_account_dba.is_exists_by<by_name>(op.better), "Account \"${1}\" must exist.", ("1", op.better));

    odds odds(op.odds.numerator, op.odds.denominator);
    const auto kind = op.live //
        ? pending_bet_kind::live
        : pending_bet_kind::non_live;

    const auto& bet
        = _betting_svc.create_pending_bet(op.better, op.stake, odds, op.wincase, game_obj.uuid, op.uuid, kind);

    auto bets_to_cancel = _betting_matcher.match(bet);

    _betting_svc.cancel_pending_bets(utils::unwrap_ref_wrapper(bets_to_cancel));
}
}
}
