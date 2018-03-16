#include "parsers.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <sstream>
#include <iostream>
#include <vector>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <fc/io/json.hpp>

namespace scorum {
namespace util {

using scorum::protocol::asset;

struct user_info
{
    asset balance_scr = asset(0, SCORUM_SYMBOL);
    asset balance_sp = asset(0, SP_SYMBOL);
};
}
}

FC_REFLECT(scorum::util::user_info, (balance_scr)(balance_sp));

namespace scorum {
namespace util {

using scorum::protocol::chain_id_type;

void load(const std::string& path, genesis_state_type& genesis)
{
    boost::filesystem::path path_to_load(path);
    FC_ASSERT(boost::filesystem::exists(path_to_load), "Path ${p} does not exists.", ("p", path_to_load.string()));

    path_to_load.normalize();

    ilog("Loading ${file}.", ("file", path_to_load.string()));

    boost::filesystem::ifstream fl;
    fl.open(path_to_load.string(), std::ios::in);

    FC_ASSERT((bool)fl, "Can't read file ${p}.", ("p", path_to_load.string()));

    std::stringstream ss;

    ss << fl.rdbuf();

    fl.close();

    genesis = fc::json::from_string(ss.str()).as<genesis_state_type>();
}

void check_users(const genesis_state_type& genesis, const std::vector<std::string>& users)
{
    ilog("Checking users '${users}'.", ("users", users));
    std::map<std::string, user_info> checked_users;
    for (const auto& user : users)
    {
        for (const auto& account : genesis.accounts)
        {
            if (account.name == user)
            {
                checked_users[user].balance_scr = account.scr_amount;
                if (checked_users.size() == users.size())
                    break;
            }
        }
        for (const auto& account : genesis.steemit_bounty_accounts)
        {
            if (account.name == user)
            {
                checked_users[user].balance_sp = account.sp_amount;
                if (checked_users.size() == users.size())
                    break;
            }
        }
    }
    for (const auto& info : checked_users)
    {
        ilog("${user}: ${info}", ("user", info.first)("info", info.second));
    }
    FC_ASSERT(checked_users.size() == users.size(), "Not all users from list exist in genesis");
}

void save_to_string(genesis_state_type& genesis, std::string& output_json, bool pretty_print)
{
    fc::variant vo;
    fc::to_variant(genesis, vo);

    if (pretty_print)
    {
        output_json = fc::json::to_pretty_string(vo);
        output_json.append("\n");
    }
    else
    {
        output_json = fc::json::to_string(vo);
    }
}

void save_to_file(genesis_state_type& genesis, const std::string& path, bool pretty_print)
{
    std::string output_json;
    save_to_string(genesis, output_json, pretty_print);

    boost::filesystem::path path_to_save(path);

    path_to_save.normalize();

    ilog("Saving ${file}.", ("file", path_to_save.string()));

    boost::filesystem::ofstream fl;
    fl.open(path_to_save);

    FC_ASSERT((bool)fl, "Can't write to file ${p}.", ("p", path_to_save.string()));

    fl << output_json;
    fl.close();
}

void print(genesis_state_type& genesis, bool pretty_print)
{
    std::string output_json;
    save_to_string(genesis, output_json, pretty_print);
    std::cout << output_json << std::endl;
}
}
}
