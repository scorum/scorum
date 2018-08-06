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
    : _dgp_property(db.dynamic_global_property_service())
    , _betting_property(db.betting_property_service())
    , _bet(db.bet_service())
{
}

bool betting_service::is_betting_moderator(const account_name_type& account_name) const
{
    try
    {
        return _betting_property.get().moderator == account_name;
    }
    FC_CAPTURE_LOG_AND_RETHROW((account_name))
}

const bet_object& betting_service::create_bet(const account_name_type& better,
                                              const game_id_type game,
                                              const wincase_type& wincase,
                                              const std::string& odds_value,
                                              const asset& stake)
{
    try
    {
        FC_ASSERT(stake.amount > 0);
        FC_ASSERT(stake.symbol() == SCORUM_SYMBOL);
        return _bet.create([&](bet_object& obj) {
            obj.created = _dgp_property.head_block_time();
            obj.better = better;
            obj.game = game;
            obj.wincase = wincase;
            obj.value = odds::from_string(odds_value);
            obj.stake = stake;
            obj.rest_stake = stake;
            obj.potential_gain = calculate_gain(obj.stake, obj.value);
        });
    }
    FC_CAPTURE_LOG_AND_RETHROW((better)(game)(wincase)(odds_value)(stake))
}
}
}
}
