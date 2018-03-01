#include "file_parser.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <sstream>
#include <string>

#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

#include <fc/io/json.hpp>

using scorum::protocol::public_key_type;
using scorum::protocol::asset;

namespace scorum {
namespace util {

struct file_format_type
{
    struct user
    {
        std::string name;
        public_key_type public_key;
        asset scr_amount;
        asset sp_amount;
    };

    std::vector<user> users;
};
}
}

FC_REFLECT(scorum::util::file_format_type::user, (name)(public_key)(scr_amount)(sp_amount))

FC_REFLECT(scorum::util::file_format_type, (users))

namespace scorum {
namespace util {

using scorum::protocol::asset;
using scorum::protocol::public_key_type;

file_parser::file_parser(const boost::filesystem::path& path)
    : _path(path)
{
    FC_ASSERT(boost::filesystem::exists(_path), "Path ${p} does not exists.", ("p", _path.string()));

    _path.normalize();
}

void file_parser::update(genesis_state_type& result)
{
    ilog("Loading ${file}.", ("file", _path.string()));

    boost::filesystem::ifstream fl;
    fl.open(_path.string(), std::ios::in);

    FC_ASSERT((bool)fl, "Can't read file ${p}.", ("p", _path.string()));

    std::stringstream ss;

    ss << fl.rdbuf();

    fl.close();

    file_format_type input = fc::json::from_string(ss.str()).as<file_format_type>();

    result.steemit_bounty_accounts.clear();

    for (const auto& user : input.users)
    {
        // TODO

        result.accounts.push_back({ user.name, "", user.public_key, user.scr_amount });
    }
}
}
}
