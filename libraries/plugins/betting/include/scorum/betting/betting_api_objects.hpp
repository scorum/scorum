#pragma once
#include <scorum/protocol/betting/game.hpp>
#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/chain/schema/game_object.hpp>

namespace scorum {
namespace betting {
namespace api {

using scorum::chain::game_id_type;
using scorum::chain::game_status;
using scorum::protocol::account_name_type;
using scorum::protocol::betting::game_type;
using scorum::protocol::betting::market_type;
using scorum::protocol::betting::wincase_type;

struct game_api_obj
{
    game_api_obj(const scorum::chain::game_object& obj)
        : id(obj.id)
        , moderator(obj.moderator)
        , name(fc::to_string(obj.name))
        , start(obj.start)
        , finish(obj.finish)
        , status(obj.status)
        , game(obj.game)
        , markets(obj.markets.cbegin(), obj.markets.cend())
        , results(obj.results.cbegin(), obj.results.cend())
    {
    }

    game_id_type id;
    account_name_type moderator;
    std::string name;
    fc::time_point_sec start;
    fc::time_point_sec finish;

    game_status status;

    game_type game;
    std::vector<market_type> markets;
    std::vector<wincase_type> results;
};
}
}
}

FC_REFLECT(scorum::betting::api::game_api_obj, (id)(moderator)(name)(start)(finish)(status)(game)(markets)(results))
