#pragma once

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

struct bet_object;

namespace betting {

using scorum::protocol::betting::wincase_pair;

struct betting_service_i
{
    virtual bool is_betting_moderator(const account_name_type& account_name) const = 0;

    virtual const bet_object& create_bet(const account_name_type& better,
                                         const game_id_type game,
                                         const wincase_type& wincase,
                                         const odds& odds_value,
                                         const asset& stake)
        = 0;

    virtual bool is_bet_matched(const bet_object& bet) const = 0;

    virtual std::vector<std::reference_wrapper<const bet_object>>
    get_bets(const game_id_type& game, const std::vector<wincase_pair>& wincase_pairs) const = 0;

    virtual void cancel_bets(const std::vector<std::reference_wrapper<const bet_object>>& bets) const = 0;
    virtual void cancel_game(const game_id_type& game_id) = 0;
};

class betting_service : public betting_service_i
{
public:
    betting_service(data_service_factory_i&);

    virtual bool is_betting_moderator(const account_name_type& account_name) const override;

    virtual const bet_object& create_bet(const account_name_type& better,
                                         const game_id_type game,
                                         const wincase_type& wincase,
                                         const odds& odds_value,
                                         const asset& stake) override;

    virtual bool is_bet_matched(const bet_object& bet) const override;

    virtual std::vector<std::reference_wrapper<const bet_object>>
    get_bets(const game_id_type& game, const std::vector<wincase_pair>& wincase_pairs) const override;

    virtual void cancel_bets(const std::vector<std::reference_wrapper<const bet_object>>& bets) const override;
    virtual void cancel_game(const game_id_type& game_id) override;

private:
    dynamic_global_property_service_i& _dgp_property_service;
    betting_property_service_i& _betting_property_service;
    matched_bet_service_i& _matched_bet_svc;
    pending_bet_service_i& _pending_bet_svc;
    bet_service_i& _bet_svc;
    game_service_i& _game_svc;
};
}
}
}
