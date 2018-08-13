#pragma once

#include <scorum/chain/schema/bet_objects.hpp>

namespace scorum {
namespace chain {

struct data_service_factory_i;

struct dynamic_global_property_service_i;
struct betting_property_service_i;
struct bet_service_i;
namespace dba {
template <typename> struct db_accessor_i;
struct db_accessor_factory;
}

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

    virtual void return_unresolved_bets(const game_object& game) = 0;
    virtual void return_bets(const game_object& game, const std::vector<wincase_pair>& cancelled_wincases) = 0;
    virtual void remove_disputs(const game_object& game) = 0;
    virtual void remove_bets(const game_object& game) = 0;
};

class betting_service : public betting_service_i
{
public:
    betting_service(data_service_factory_i&, dba::db_accessor_factory&);

    virtual bool is_betting_moderator(const account_name_type& account_name) const override;

    virtual const bet_object& create_bet(const account_name_type& better,
                                         const game_id_type game,
                                         const wincase_type& wincase,
                                         const odds& odds_value,
                                         const asset& stake) override;

    virtual void return_unresolved_bets(const game_object& game) override;
    virtual void return_bets(const game_object& game, const std::vector<wincase_pair>& cancelled_wincases) override;
    virtual void remove_disputs(const game_object& game) override;
    virtual void remove_bets(const game_object& game) override;

private:
    dynamic_global_property_service_i& _dgp_property_service;
    dba::db_accessor_i<betting_property_object>& _betting_property_dba;
    dba::db_accessor_i<bet_object>& _bet_dba;
};
}
}
}
