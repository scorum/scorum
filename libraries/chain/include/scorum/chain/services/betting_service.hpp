#pragma once

#include <scorum/chain/services/dbs_base.hpp>

#include <scorum/protocol/odds.hpp>
#include <scorum/chain/schema/bet_objects.hpp>

// This is the new none dbs-service
// Look BLOC-415 and refactor this class
// ===========================================
// It must not inherit dbs_base,
// replace dbs_betting to betting_service_impl

namespace scorum {
namespace chain {
struct betting_service_i
{
    virtual bool is_betting_moderator(const account_name_type& account_name) const = 0;

    virtual const bet_object& create_bet(const account_name_type& better,
                                         const game_id_type game,
                                         const wincase_type& wincase,
                                         const std::string& odds_value,
                                         const asset& stake)
        = 0;

    // match bet with existen pending bets or create pending bet
    virtual void match(const bet_object& bet) = 0;

    // how many assets win and returned to accunt
    virtual asset get_gain(const bet_object& bet) const = 0;

    // how many asset returned to accunt
    virtual asset get_rest(const bet_object& bet) const = 0;
};

using scorum::protocol::odds;

struct dynamic_global_property_service_i;
struct betting_property_service_i;
struct bet_service_i;
struct pending_bet_service_i;
struct matched_bet_service_i;

asset calculate_matched_stake(const asset& bet1_stake,
                              const asset& bet2_stake,
                              const odds& bet1_odds,
                              const odds& bet2_odds);

class dbs_betting : public dbs_base, public betting_service_i
{
    friend class dbservice_dbs_factory;

protected:
    dbs_betting(database& db);

public:
    virtual bool is_betting_moderator(const account_name_type& account_name) const override;

    virtual const bet_object& create_bet(const account_name_type& better,
                                         const game_id_type game,
                                         const wincase_type& wincase,
                                         const std::string& odds_value,
                                         const asset& stake) override;

    virtual void match(const bet_object& bet) override;

    virtual asset get_gain(const bet_object& bet) const override;

    virtual asset get_rest(const bet_object& bet) const override;

private:
    bool is_bets_matched(const bet_object& bet1, const bet_object& bet2);

    dynamic_global_property_service_i& _dgp_property;
    betting_property_service_i& _betting_property;
    bet_service_i& _bet;
    pending_bet_service_i& _pending_bet;
    matched_bet_service_i& _matched_bet;
};
}
}
