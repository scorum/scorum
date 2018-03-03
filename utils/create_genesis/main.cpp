#include <iostream>

#include <boost/program_options.hpp>

#include <scorum/app/api.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <fc/exception/exception.hpp>

#include "file_parser.hpp"
#ifdef BUILD_MONGODB_CLI
#include "mongo_parser.hpp"
#endif

#include "genesis_tester.hpp"

using scorum::chain::genesis_state_type;

int main(int argc, char** argv)
{
    try
    {
        namespace bpo = boost::program_options;

        bpo::options_description opts;
        // clang-format off
        opts.add_options()
                ("help,h", "Print this help message and exit.")
                ("version,v", "Print version number and exit.")
#ifdef BUILD_MONGODB_CLI
                ("mongodb-endpoint,m", bpo::value<std::string>()->default_value("mongodb://localhost:27017"), "Server MongoDB connection string")
#endif
                ("import-json,i",     bpo::value<std::string>(), "Path for Json data file to parse.")
                ("input-genesis-json,g",     bpo::value<std::string>(), "Path for Json genesis file to concatenate with result.")
                ("test-resut-genesis,t", "Test opening sandbox database by resulted genesis.")
                ("shared-memory-reserved-size,m",  bpo::value<unsigned int>()->default_value(100), "Reserved disk size (Mb) for database test.")
                ("output-genesis-json,o", bpo::value<std::string>(), "Path for result Json genesis file.");
        // clang-format on

        bpo::variables_map options;

        bpo::store(bpo::parse_command_line(argc, argv, opts), options);

        if (options.count("version"))
        {
            scorum::app::print_application_version();
            return 0;
        }

        if (options.count("help"))
        {
            std::cout << opts << std::endl;
            return 0;
        }

        genesis_state_type genesis;

        if (options.count("input-genesis-json"))
        {
            scorum::util::load(options.at("input-genesis-json").as<std::string>(), genesis);
        }

        if (options.count("import-json"))
        {
            scorum::util::file_parser fl(options.at("import-json").as<std::string>());

            fl.update(genesis);
        }
        else
        {
            std::string connection_uri;
            if (options.count("mongodb-endpoint"))
            {
                connection_uri = options.at("mongodb-endpoint").as<std::string>();
            }

#ifdef BUILD_MONGODB_CLI
            scorum::util::mongo_parser mongo(connection_uri);

            mongo.update(genesis);
#else
            std::cout << opts << std::endl;
            return 1;
#endif
        }

        if (options.count("test-resut-genesis"))
        {
            unsigned int shared_mem_mb_size = options.at("shared-memory-reserved-size").as<unsigned int>();
            scorum::util::test_database(genesis, shared_mem_mb_size);
        }

        if (options.count("output-genesis-json"))
        {
            scorum::util::save_to_file(genesis, options.at("output-genesis-json").as<std::string>());
        }
        else
        {
            scorum::util::print(genesis);
        }

        return 0;
    }
    FC_CAPTURE_AND_LOG(())

    return 1;
}
