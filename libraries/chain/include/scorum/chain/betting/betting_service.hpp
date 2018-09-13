#pragma once
#include <vector>
#include <functional>
#include <scorum/chain/schema/bet_objects.hpp>

namespace scorum {
namespace chain {

struct data_service_factory_i;

struct dynamic_global_property_service_i;
struct betting_property_service_i;
struct bet_service_i;
struct matched_bet_service_i;
struct pending_bet_service_i;
struct game_service_i;
struct account_service_i;

using scorum::protocol::market_type;
using scorum::protocol::wincase_type;

struct betting_service_i
{
    // TODO: will be removed after db_accessors introduction
    using bet_crefs_type = std::vector<std::reference_wrapper<const bet_object>>;
    using pending_bet_crefs_type = std::vector<std::reference_wrapper<const pending_bet_object>>;

    virtual bool is_betting_moderator(const account_name_type& account_name) const = 0;

    virtual const bet_object& create_bet(const account_name_type& better,
                                         const game_id_type game,
                                         const wincase_type& wincase,
                                         const odds& odds_value,
                                         const asset& stake)
        = 0;

    virtual void cancel_game(const game_id_type& game_id) = 0;
    virtual void cancel_bets(const game_id_type& game_id) = 0;
    virtual void cancel_bets(const game_id_type& game_id, const std::vector<market_type>& cancelled_markets) = 0;
    virtual void cancel_pending_bets(const game_id_type& game_id) = 0;
    virtual void cancel_pending_bets(const game_id_type& game_id, pending_bet_kind kind) = 0;
    virtual void cancel_matched_bets(const game_id_type& game_id) = 0;

    virtual bool is_bet_matched(const bet_object& bet) const = 0;
};

class betting_service : public betting_service_i
{
public:
    betting_service(data_service_factory_i&);

    bool is_betting_moderator(const account_name_type& account_name) const override;

    const bet_object& create_bet(const account_name_type& better,
                                 const game_id_type game,
                                 const wincase_type& wincase,
                                 const odds& odds_value,
                                 const asset& stake) override;

    void cancel_game(const game_id_type& game_id) override;
    void cancel_bets(const game_id_type& game_id) override;
    void cancel_bets(const game_id_type& game_id, const std::vector<market_type>& cancelled_markets) override;
    void cancel_pending_bets(const game_id_type& game_id) override;
    void cancel_pending_bets(const game_id_type& game_id, pending_bet_kind kind) override;
    void cancel_matched_bets(const game_id_type& game_id) override;

    bool is_bet_matched(const bet_object& bet) const override;

private:
    // TODO: signature will be changed (after db_accessors introduction)
    void cancel_bets(const bet_crefs_type& bets);
    void cancel_pending_bets(const pending_bet_crefs_type& pending_bets);

private:
    dynamic_global_property_service_i& _dgp_property_service;
    betting_property_service_i& _betting_property_service;
    matched_bet_service_i& _matched_bet_svc;
    pending_bet_service_i& _pending_bet_svc;
    bet_service_i& _bet_svc;
    game_service_i& _game_svc;
    account_service_i& _account_svc;
};
}
}
