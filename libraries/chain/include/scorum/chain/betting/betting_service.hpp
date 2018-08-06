#pragma once

#include <scorum/chain/schema/bet_objects.hpp>

namespace scorum {
namespace chain {

struct data_service_factory_i;

struct dynamic_global_property_service_i;
struct betting_property_service_i;
struct bet_service_i;

namespace betting {

struct betting_service_i
{
    virtual bool is_betting_moderator(const account_name_type& account_name) const = 0;

    virtual const bet_object& create_bet(const account_name_type& better,
                                         const game_id_type game,
                                         const wincase_type& wincase,
                                         const std::string& odds_value,
                                         const asset& stake)
        = 0;
};

class betting_service : public betting_service_i
{
public:
    betting_service(data_service_factory_i&);

    virtual bool is_betting_moderator(const account_name_type& account_name) const override;

    virtual const bet_object& create_bet(const account_name_type& better,
                                         const game_id_type game,
                                         const wincase_type& wincase,
                                         const std::string& odds_value,
                                         const asset& stake) override;

private:
    dynamic_global_property_service_i& _dgp_property;
    betting_property_service_i& _betting_property;
    bet_service_i& _bet;
};
}
}
}
