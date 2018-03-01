#include "parsers.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <sstream>
#include <iostream>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <fc/io/json.hpp>

namespace scorum {
namespace util {

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

void save_to_string(const genesis_state_type& genesis, std::string& output_json, bool pretty_print)
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

void save_to_file(const genesis_state_type& genesis, const std::string& path, bool pretty_print)
{
    std::string output_json;
    save_to_string(genesis, output_json, pretty_print);

    boost::filesystem::path path_to_save(path);
    boost::filesystem::ofstream fl;
    fl.open(path_to_save);

    FC_ASSERT((bool)fl, "Can't write to file ${p}.", ("p", path_to_save.string()));

    fl << output_json;
    fl.close();
}

void print(const genesis_state_type& genesis)
{
    std::string output_json;
    save_to_string(genesis, output_json, true);
    std::cout << output_json << std::endl;
}
}
}
