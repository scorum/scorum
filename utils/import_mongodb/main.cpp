#include <iostream>
#include <string>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <scorum/protocol/asset.hpp>

using scorum::chain::genesis_state_type;
using scorum::protocol::asset;

int main(int, char**)
{
    mongocxx::instance inst{};
    mongocxx::client conn{ mongocxx::uri{ "mongodb://localhost:27017" } };

    auto collection = conn["INVESTORS"]["investors"];

    auto cursor = collection.find({});

    genesis_state_type genesis;
    for (auto&& doc : cursor)
    {
        genesis.development_scr_supply = asset(11, SCORUM_SYMBOL);
        genesis.development_sp_supply = asset(22, VESTS_SYMBOL);
        std::cout << bsoncxx::to_json(doc) << std::endl;
    }

    fc::variant vo;
    fc::to_variant(genesis, vo);

    std::cout << fc::json::to_string(vo) << std::endl;
}
