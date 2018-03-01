#ifdef BUILD_MONGODB_CLI

#include "mongo_parser.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace util {

struct document_format_type;

// TODO
}
}

namespace scorum {
namespace util {

mongo_parser::mongo_parser(const std::string& connection_uri)
    : _connection_uri(connection_uri)
{
}

void mongo_parser::update(genesis_state_type& result)
{
    mongocxx::instance inst{};
    mongocxx::client conn{ mongocxx::uri{ _connection_uri } };

    auto collection = conn["INVESTORS"]["investors"];

    auto cursor = collection.find({}); // request: TODO

    result.steemit_bounty_accounts.clear();

    for (auto&& doc : cursor)
    {
        process_document(bsoncxx::to_json(doc), result);
    }
}

void mongo_parser::process_document(const std::string& doc_json, genesis_state_type& result)
{
    // TODO
}
}
}

#endif // BUILD_MONGODB_CLI
