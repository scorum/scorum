#include <scorum/app/application.hpp>

#include <scorum/witness/witness_plugin.hpp>
#include <scorum/manifest/plugins.hpp>

#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>

#include <scorum/protocol/version.hpp>

#include <boost/filesystem.hpp>
#include <fstream>

#ifdef WIN32
#include <signal.h>
#else
#include <csignal>
#endif
#include <graphene/utilities/key_conversion.hpp>

#include "log_configurator.hpp"

#define SCORUM_DAEMON_DEFAULT_CONFIG_FILE_NAME "config.ini"

using namespace scorum;
using scorum::protocol::version;
namespace bpo = boost::program_options;

int main(int argc, char** argv)
{
    scorum::plugin::initialize_plugin_factories();
    app::application* node = new app::application();
    fc::oexception unhandled_exception;
    try
    {
        // clang-format off
        bpo::options_description app_options("Scorum Daemon");
        bpo::options_description cfg_options("Scorum Daemon");
        app_options.add_options()
                ("help,h", "Print this help message and exit.")
                ("config-file", bpo::value<boost::filesystem::path>(),
                 "Path to config file. Defaults to data_dir/" SCORUM_DAEMON_DEFAULT_CONFIG_FILE_NAME);
        // clang-format on

        bpo::variables_map options;

        for (const std::string& plugin_name : scorum::plugin::get_available_plugins())
            node->register_abstract_plugin(scorum::plugin::create_plugin(plugin_name, node));

        try
        {
            bpo::options_description cli, cfg;
            node->set_program_options(cli, cfg);

            app_options.add(cli);
            cfg_options.add(cfg);

            logger::set_logging_program_options(cfg_options);

            bpo::store(bpo::parse_command_line(argc, argv, app_options), options);
        }
        catch (const boost::program_options::error& e)
        {
            std::cerr << "Error parsing command line: " << e.what() << "\n";
            return 1;
        }

        if (options.count("version"))
        {
            app::print_application_version();
            return 0;
        }

        if (options.count("help"))
        {
            std::cout << app_options << "\n";
            return 0;
        }

        fc::path data_dir;
        if (options.count("data-dir"))
        {
            data_dir = options["data-dir"].as<boost::filesystem::path>();
            if (data_dir.is_relative())
                data_dir = fc::current_path() / data_dir;
        }

        fc::path config_ini_path = data_dir / SCORUM_DAEMON_DEFAULT_CONFIG_FILE_NAME;
        if (options.count("config-file"))
        {
            config_ini_path = options["config-file"].as<boost::filesystem::path>();
        }

        if (!fc::exists(config_ini_path))
        {
            ilog("Writing new config file at ${path}", ("path", config_ini_path));
            if (!fc::exists(data_dir))
                fc::create_directories(data_dir);

            std::ofstream out_cfg(config_ini_path.preferred_string());

            scorum::app::print_program_options(out_cfg, cfg_options);

            out_cfg.close();
        }

        ilog("Using config file ${path}", ("path", config_ini_path));

        // get the basic options
        bpo::store(bpo::parse_config_file<char>(config_ini_path.preferred_string().c_str(), cfg_options, true),
                   options);

        // try to get logging options from the config file.
        try
        {
            fc::optional<fc::logging_config> logging_config
                = logger::load_logging_config_from_options(options, data_dir);

            if (logging_config)
                fc::configure_logging(*logging_config);
        }
        catch (const fc::exception&)
        {
            wlog("Error parsing logging config from config file ${config}, using default config",
                 ("config", config_ini_path.preferred_string()));
        }

        std::cerr << "------------------------------------------------------\n\n";
        if (!options.count("read-only"))
        {
            std::cerr << "            STARTING SCORUM NETWORK\n\n";
        }
        else
        {
            std::cerr << "            READONLY NODE\n\n";
        }
        std::cerr << "------------------------------------------------------\n";
        std::cerr << "blockchain version: " << fc::string(SCORUM_BLOCKCHAIN_VERSION) << "\n";
        std::cerr << "------------------------------------------------------\n";

        ilog("parsing options");
        bpo::notify(options);

        app::print_application_version();

        std::cout << "Starting Scorum network...\n\n";

        ilog("initializing node");
        node->initialize(options);
        ilog("initializing plugins");
        node->initialize_plugins(options);

        ilog("starting node");
        node->startup();
        ilog("starting plugins");
        node->startup_plugins();

        fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");

        fc::set_signal_handler(
            [&exit_promise](int signal) {
                elog("Caught SIGINT attempting to exit cleanly");
                exit_promise->set_value(signal);
            },
            SIGINT);

        fc::set_signal_handler(
            [&exit_promise](int signal) {
                elog("Caught SIGTERM attempting to exit cleanly");
                exit_promise->set_value(signal);
            },
            SIGTERM);

        node->chain_database()->with_read_lock([&]() {
            ilog("Started node on a chain with ${h} blocks.", ("h", node->chain_database()->head_block_num()));
        });

        std::cout << "Scorum network started.\n\n";

        exit_promise->wait();
        node->shutdown_plugins();
        node->shutdown();
        delete node;
        return 0;
    }
    catch (const fc::exception& e)
    {
        // deleting the node can yield, so do this outside the exception handler
        unhandled_exception = e;
    }

    if (unhandled_exception)
    {
        elog("Exiting with error:\n${e}", ("e", unhandled_exception->to_detail_string()));
        node->shutdown();
        delete node;
        return 1;
    }
    ilog("done");
    return 0;
}
