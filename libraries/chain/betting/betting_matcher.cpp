#include <scorum/chain/betting/betting_matcher.hpp>

#include <scorum/chain/database/database_virtual_operations.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/betting/betting_math.hpp>
#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/protocol/betting/market.hpp>

namespace scorum {
namespace chain {

bool can_be_matched(const pending_bet_object& bet)
{
    return bet.data.stake * bet.data.bet_odds > bet.data.stake;
}

betting_matcher_i::~betting_matcher_i()
{
}

betting_matcher::betting_matcher(database_virtual_operations_emmiter_i& virt_op_emitter,
                                 dba::db_accessor<pending_bet_object>& pending_bet_dba,
                                 dba::db_accessor<matched_bet_object>& matched_bet_dba,
                                 dba::db_accessor<dynamic_global_property_object>& dprop_dba)
    : _virt_op_emitter(virt_op_emitter)
    , _pending_bet_dba(pending_bet_dba)
    , _matched_bet_dba(matched_bet_dba)
    , _dprop_dba(dprop_dba)
{
}

std::vector<std::reference_wrapper<const pending_bet_object>>
betting_matcher::match(const pending_bet_object& bet2, const fc::time_point_sec& head_block_time)
{
    try
    {
        std::vector<std::reference_wrapper<const pending_bet_object>> bets_to_cancel;

        auto key = std::make_tuple(bet2.game, create_opposite(bet2.get_wincase()));
        auto pending_bets = _pending_bet_dba.get_range_by<by_game_id_wincase>(key);

        for (const auto& bet1 : pending_bets)
        {
            if (!is_bets_matched(bet1, bet2))
                continue;

            auto matched
                = calculate_matched_stake(bet1.data.stake, bet2.data.stake, bet1.data.bet_odds, bet2.data.bet_odds);

            if (matched.bet1_matched.amount > 0 && matched.bet2_matched.amount > 0)
            {
                const auto matched_bet_id = create_matched_bet(_matched_bet_dba, bet1, bet2, matched, head_block_time);

                _pending_bet_dba.update(bet1, [&](pending_bet_object& o) { o.data.stake -= matched.bet1_matched; });
                _pending_bet_dba.update(bet2, [&](pending_bet_object& o) { o.data.stake -= matched.bet2_matched; });

                _dprop_dba.update([&](dynamic_global_property_object& obj) {
                    obj.betting_stats.matched_bets_volume += matched.bet1_matched + matched.bet2_matched;
                    obj.betting_stats.pending_bets_volume -= matched.bet1_matched + matched.bet2_matched;
                });

                _virt_op_emitter.push_virtual_operation(protocol::bets_matched_operation(
                    bet1.data.better, bet2.data.better, bet1.get_uuid(), bet2.get_uuid(), matched.bet1_matched,
                    matched.bet2_matched, matched_bet_id));
            }

            if (!can_be_matched(bet1))
            {
                bets_to_cancel.emplace_back(bet1);
            }

            if (!can_be_matched(bet2))
            {
                bets_to_cancel.emplace_back(bet2);
                break;
            }
        }

        return bets_to_cancel;
    }
    FC_CAPTURE_LOG_AND_RETHROW((bet2))
}

bool betting_matcher::is_bets_matched(const pending_bet_object& bet1, const pending_bet_object& bet2) const
{
    return bet1.data.bet_odds.inverted() == bet2.data.bet_odds;
}

int64_t create_matched_bet(dba::db_accessor<matched_bet_object>& matched_bet_dba,
                           const pending_bet_object& bet1,
                           const pending_bet_object& bet2,
                           const matched_stake_type& matched,
                           fc::time_point_sec head_block_time)
{
    FC_ASSERT(bet1.game == bet2.game, "bets game id is not equal.");
    FC_ASSERT(bet1.get_wincase() == create_opposite(bet2.get_wincase()));

    return matched_bet_dba
        .create([&](matched_bet_object& obj) {
            obj.bet1_data = bet1.data;
            obj.bet2_data = bet2.data;
            obj.bet1_data.stake = matched.bet1_matched;
            obj.bet2_data.stake = matched.bet2_matched;
            obj.market = bet1.market;
            obj.game = bet1.game;
            obj.created = head_block_time;
        })
        .id._id;
}

} // namespace chain
} // namespace scorum
