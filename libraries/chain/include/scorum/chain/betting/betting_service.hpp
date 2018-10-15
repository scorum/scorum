#pragma once
#include <vector>
#include <functional>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/utils/any_range.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::market_type;

struct data_service_factory_i;

struct dynamic_global_property_service_i;
struct betting_property_service_i;
struct game_service_i;
struct account_service_i;
struct database_virtual_operations_emmiter_i;

class betting_property_object;
class pending_bet_object;
class matched_bet_object;

namespace dba {
template <typename> struct db_accessor;
}

struct betting_service_i
{
    virtual bool is_betting_moderator(const account_name_type& account_name) const = 0;

    virtual void cancel_game(game_id_type game_id) = 0;
    virtual void cancel_bets(game_id_type game_id) = 0;
    virtual void cancel_bets(game_id_type game_id, fc::time_point_sec created_from) = 0;
    virtual void cancel_bets(game_id_type game_id, const fc::flat_set<market_type>& cancelled_markets) = 0;

    virtual void cancel_pending_bet(pending_bet_id_type id) = 0;
    virtual void cancel_pending_bets(game_id_type game_id) = 0;
    virtual void cancel_pending_bets(game_id_type game_id, pending_bet_kind kind) = 0;
    virtual void cancel_pending_bets(utils::bidir_range<const pending_bet_object> bets, uuid_type game_uuid) = 0;

    virtual void cancel_matched_bets(game_id_type game_id) = 0;
    virtual void cancel_matched_bets(utils::bidir_range<const matched_bet_object> bets, uuid_type game_uuid) = 0;
};

class betting_service : public betting_service_i
{
public:
    betting_service(data_service_factory_i&,
                    database_virtual_operations_emmiter_i&,
                    dba::db_accessor<betting_property_object>&,
                    dba::db_accessor<matched_bet_object>&,
                    dba::db_accessor<pending_bet_object>&,
                    dba::db_accessor<game_object>&);

    bool is_betting_moderator(const account_name_type& account_name) const override;

    void cancel_game(game_id_type game_id) override;
    void cancel_bets(game_id_type game_id) override;
    void cancel_bets(game_id_type game_id, fc::time_point_sec created_from) override;
    void cancel_bets(game_id_type game_id, const fc::flat_set<market_type>& cancelled_markets) override;

    void cancel_pending_bet(pending_bet_id_type id) override;
    void cancel_pending_bets(game_id_type game_id) override;
    void cancel_pending_bets(game_id_type game_id, pending_bet_kind kind) override;
    void cancel_pending_bets(utils::bidir_range<const pending_bet_object> bets, uuid_type game_uuid) override;

    void cancel_matched_bets(game_id_type game_id) override;
    void cancel_matched_bets(utils::bidir_range<const matched_bet_object> bets, uuid_type game_uuid) override;

private:
    void
    return_or_restore_bet(const bet_data& bet, game_id_type game_id, uuid_type game_uuid, fc::time_point_sec threshold);
    void push_matched_bet_cancelled_op(const bet_data& bet, uuid_type game_uuid);
    void push_pending_bet_cancelled_op(const bet_data& bet, uuid_type game_uuid);

    dynamic_global_property_service_i& _dgp_property_service;
    account_service_i& _account_svc;
    database_virtual_operations_emmiter_i& _virt_op_emitter;
    dba::db_accessor<betting_property_object>& _betting_property_dba;
    dba::db_accessor<matched_bet_object>& _matched_bet_dba;
    dba::db_accessor<pending_bet_object>& _pending_bet_dba;
    dba::db_accessor<game_object>& _game_dba;
};
}
}
