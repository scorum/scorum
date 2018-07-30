#pragma once
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/services/service_base.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>

namespace scorum {
namespace chain {

struct game_service_i : public base_service_i<game_object>
{
    virtual const game_object& create(const account_name_type& moderator,
                                      const std::string& game_name,
                                      fc::time_point_sec start,
                                      const betting::game_type& game,
                                      const std::vector<betting::market_type>& markets)
        = 0;

    virtual const game_object* find(const std::string& game_name) const = 0;
};

class dbs_game : public dbs_service_base<game_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_game(database& db);

public:
    virtual const game_object& create(const account_name_type& moderator,
                                      const std::string& game_name,
                                      fc::time_point_sec start,
                                      const betting::game_type& game,
                                      const std::vector<betting::market_type>& markets) override;
    virtual const game_object* find(const std::string& game_name) const override;
};
}
}