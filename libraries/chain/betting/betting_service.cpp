#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>

#include <scorum/protocol/betting/wincase_comparison.hpp>

#include <scorum/chain/betting/betting_math.hpp>

namespace scorum {
namespace chain {
namespace betting {

betting_service::betting_service(data_service_factory_i& db)
    : _dgp_property_service(db.dynamic_global_property_service())
    , _betting_property_service(db.betting_property_service())
    , _bet_service(db.bet_service())
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
        return _bet_service.create([&](bet_object& obj) {
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

void betting_service::return_unresolved_bets(const game_object& game)
{
    boost::ignore_unused_variable_warning(game);
    FC_THROW("not implemented");
}

void betting_service::return_bets(const game_object& game, const std::vector<wincase_pair>& cancelled_wincases)
{
    boost::ignore_unused_variable_warning(game);
    boost::ignore_unused_variable_warning(cancelled_wincases);
    FC_THROW("not implemented");
}

void betting_service::remove_disputs(const game_object& game)
{
    boost::ignore_unused_variable_warning(game);
    FC_THROW("not implemented");
}

void betting_service::remove_bets(const game_object& game)
{
    boost::ignore_unused_variable_warning(game);
    FC_THROW("not implemented");
}
}
}
}
