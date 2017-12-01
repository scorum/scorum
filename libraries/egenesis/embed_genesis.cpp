/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/filesystem/fstream.hpp>

#include <boost/filesystem/fstream.hpp>

#include <fc/filesystem.hpp>
#include <fc/smart_ref_impl.hpp> // required for gcc in release mode
#include <fc/string.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <scorum/chain/genesis_state.hpp>
#include <scorum/protocol/types.hpp>

namespace sp = scorum::protocol;
namespace sc = scorum::chain;

static const std::string generated_file_banner = "// +---------------------------------------------+\n"
                                                 "// | This file was automatically generated       |\n"
                                                 "// | It will be overwritten by the build process |\n"
                                                 "// +---------------------------------------------+\n";

// clang-format off

namespace scorum { namespace app { namespace detail {
sc::genesis_state_type create_example_genesis();
}}}

// clang-format on

fc::path get_path(const boost::program_options::variables_map& options, const std::string& name)
{
    fc::path result = options[name].as<boost::filesystem::path>();
    if (result.is_relative())
        result = fc::current_path() / result;
    return result;
}

void convert_to_c_array(const std::string& src, std::string& dest, int width = 40)
{
    dest.reserve(src.length() * 6 / 5);
    bool needs_comma = false;
    int row = 0;
    for (std::string::size_type i = 0; i < src.length(); i += width)
    {
        std::string::size_type j = std::min(i + width, src.length());
        if (needs_comma)
            dest.append(",\n");
        dest.append("\"");
        for (std::string::size_type k = i; k < j; k++)
        {
            char c = src[k];
            // clang-format off
            switch(c)
            {
               // use most short escape sequences
               case '\"': dest.append("\\\""); break;
               case '\?': dest.append("\\\?"); break;
               case '\\': dest.append("\\\\"); break;
               case '\a': dest.append( "\\a"); break;
               case '\b': dest.append( "\\b"); break;
               case '\f': dest.append( "\\f"); break;
               case '\n': dest.append( "\\n"); break;
               case '\r': dest.append( "\\r"); break;
               case '\t': dest.append( "\\t"); break;
               case '\v': dest.append( "\\v"); break;

               // single quote and misc. ASCII is OK
               case '\'':
               case ' ': case '!': case '#': case '$': case '%': case '&': case '(': case ')':
               case '*': case '+': case ',': case '-': case '.': case '/':
               case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
               case ':': case ';': case '<': case '=': case '>': case '@':
               case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
               case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
               case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
               case '[': case ']': case '^': case '_': case '`':
               case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
               case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
               case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
               case '{': case '|': case '}': case '~':
                  dest.append(&c, 1);
                  break;

               // use shortest octal escape for everything else
               default:
                  dest.append("\\");
                  char dg[3];
                  dg[0] = '0' + ((c >> 6) & 3);
                  dg[1] = '0' + ((c >> 3) & 7);
                  dg[2] = '0' + ((c     ) & 7);
                  int start = (dg[0] == '0' ? (dg[1] == '0' ? 2 : 1) : 0);
                  dest.append( dg+start, 3-start );
            }
            // clang-format on
        }
        dest.append("\"");
        needs_comma = true;
        row++;
    }
}

struct egenesis_info
{
    fc::optional<sc::genesis_state_type> genesis;
    fc::optional<sp::chain_id_type> chain_id;
    fc::optional<std::string> genesis_json;
    fc::optional<fc::sha256> genesis_json_hash;
    fc::optional<std::string> genesis_json_array;
    int genesis_json_array_width;
    int genesis_json_array_height;

    void fillin()
    {
        // must specify either genesis_json or genesis
        if (genesis.valid())
        {
            if (!genesis_json.valid())
                // If genesis_json not exist, generate from genesis
                genesis_json = fc::json::to_string(*genesis);
        }
        else if (genesis_json.valid())
        {
            // If genesis not exist, generate from genesis_json
            genesis = fc::json::from_string(*genesis_json).as<sc::genesis_state_type>();
        }
        else
        {
            // Neither genesis nor genesis_json exists, crippled
            std::cerr << "embed_genesis: Need genesis or genesis_json\n";
            exit(1);
        }

        if (!genesis_json_hash.valid())
            genesis_json_hash = fc::sha256::hash(*genesis_json);

        if (!chain_id.valid())
            chain_id = genesis_json_hash;

        // init genesis_json_array from genesis_json
        if (!genesis_json_array.valid())
        {
            genesis_json_array = std::string();
            // TODO: gzip
            int width = 40;
            convert_to_c_array(*genesis_json, *genesis_json_array, width);
            int height = (genesis_json->length() + width - 1) / width;
            genesis_json_array_width = width;
            genesis_json_array_height = height;
        }
    }
};

void load_genesis(const boost::program_options::variables_map& options, egenesis_info& info)
{
    if (options.count("genesis-json"))
    {
        fc::path genesis_json_filename = get_path(options, "genesis-json");
        std::cout << "embed_genesis: Reading genesis from file " << genesis_json_filename.preferred_string() << "\n";
        info.genesis_json = std::string();
        fc::read_file_contents(genesis_json_filename, *info.genesis_json);
    }
    else
    {
        sc::genesis_state_type genesis;
        scorum::chain::utils::generate_default_genesis_state(genesis);
        info.genesis = genesis;
    }
}

bool write_to_file(const boost::filesystem::path& path, const std::string& content)
{
    if (!boost::filesystem::exists(path.parent_path()))
    {
        std::cerr << "embed_genesis: path don't exist " << path.parent_path() << std::endl;
        std::cerr << "embed_genesis: failure opening " << path << std::endl;
        return false;
    }

    boost::filesystem::ofstream outfile(path);

    if (!outfile)
    {
        std::cerr << "embed_genesis: failure opening " << path << std::endl;
        return false;
    }

    outfile << content;
    outfile.close();

    return true;
}

bool read_from_file(const boost::filesystem::path& path, std::string& template_content)
{
    if (!boost::filesystem::exists(path))
    {
        std::cerr << "embed_genesis: file does not exists " << path.c_str() << std::endl;
        return false;
    }

    boost::filesystem::ifstream f(path, std::ios::in | std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    template_content = ss.str();

    return true;
}

std::string process_template(const std::string& tmpl_content, const fc::variant_object& context)
{
    return fc::format_string(tmpl_content, context);
}

int main(int argc, char** argv)
{
    namespace bpo = boost::program_options;
    namespace bfs = boost::filesystem;

    int main_return = 0;
    bpo::options_description cli_options("Scorum Chain Identifier");

    // clang-format off
    cli_options.add_options()
            ("help,h", "Print this help message and exit.")
            ("genesis-json,g", bpo::value<bfs::path>(), "File to read genesis state from")

            ("tmplsub,t", bpo::value<std::vector<std::string>>()->composing(),
             "Given argument of form src.cpp.tmpl---dest.cpp, write dest.cpp expanding template invocations in src");
    // clang-format on

    bpo::variables_map options;

    try
    {
        bpo::store(bpo::parse_command_line(argc, argv, cli_options), options);
    }
    catch (const bpo::error& e)
    {
        std::cerr << "embed_genesis: error parsing command line: " << e.what() << "\n";
        return 1;
    }

    if (options.count("help"))
    {
        std::cout << cli_options << "\n";
        return 0;
    }

    egenesis_info info;

    load_genesis(options, info);
    info.fillin();

    fc::mutable_variant_object template_context = fc::mutable_variant_object()("chain_id", (*info.chain_id).str());

    if (info.genesis_json.valid())
    {
        template_context["generated_file_banner"] = generated_file_banner;

        template_context["genesis_json_length"] = info.genesis_json->length();
        template_context["genesis_json_array"] = (*info.genesis_json_array);
        template_context["genesis_json_hash"] = (*info.genesis_json_hash).str();
        template_context["genesis_json_array_width"] = info.genesis_json_array_width;
        template_context["genesis_json_array_height"] = info.genesis_json_array_height;
    }

    for (const std::string& src_dest : options["tmplsub"].as<std::vector<std::string>>())
    {
        std::cout << "embed_genesis: parsing tmplsub parameter \"" << src_dest << "\"\n";
        const size_t pos = src_dest.find("---");

        if (pos == std::string::npos)
        {
            std::cerr << "embed_genesis: could not parse tmplsub parameter:  '---' not found\n";
            main_return = 1;
            continue;
        }

        try
        {
            const boost::filesystem::path src_path = src_dest.substr(0, pos);
            const boost::filesystem::path dst_path = src_dest.substr(pos + 3);

            std::string template_content;

            if (!read_from_file(src_path, template_content))
            {
                main_return = 1;
                continue;
            }

            if (!write_to_file(dst_path, process_template(template_content, template_context)))
            {
                main_return = 1;
                continue;
            }
        }
        FC_LOG_AND_RETHROW()
    }

    return main_return;
}
