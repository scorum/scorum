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
                                      const fc::flat_set<betting::market_type>& markets)
        = 0;

    virtual void update_markets(const game_object& game, const fc::flat_set<betting::market_type>& markets) = 0;

    virtual bool is_exists(const std::string& game_name) const = 0;
    virtual bool is_exists(int64_t game_id) const = 0;

    virtual const game_object& get(const std::string& game_name) const = 0;
    virtual const game_object& get(int64_t game_id) const = 0;
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
                                      const fc::flat_set<betting::market_type>& markets) override;
    virtual void update_markets(const game_object& game, const fc::flat_set<betting::market_type>& markets) override;

    virtual bool is_exists(const std::string& game_name) const override;
    virtual bool is_exists(int64_t game_id) const override;

    virtual const game_object& get(const std::string& game_name) const override;
    virtual const game_object& get(int64_t game_id) const override;
};
}
}
