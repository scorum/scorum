#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>

#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/gelf_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/io/json.hpp>
#include <fc/network/ip.hpp>
#include <fc/exception/exception.hpp>

#include <scorum/app/application.hpp>
#include <scorum/app/log_configurator.hpp>
#include <scorum/protocol/config.hpp>

#define LOG_APPENDER "log-appender"
#define LOGGER "log-logger"
#define DEFAULT_GELF_APPENDER_PORT 12201

namespace bpo = boost::program_options;

using scorum::protocol::version;

struct appender_args
{
    std::string appender;
    std::string stream;
};

struct file_appender_args : public appender_args
{
    size_t rotation_interval_minutes = 60;
    size_t rotation_limit_hours = 30 * 24;
};

struct gelf_appender_args : public appender_args
{
    std::string host_name;
    std::string additional_info;
};

struct logger_args
{
    std::string name;
    std::string level;
    std::string appender;
};

FC_REFLECT(appender_args, (appender)(stream))
FC_REFLECT_DERIVED(file_appender_args, (appender_args), (rotation_interval_minutes)(rotation_limit_hours))
FC_REFLECT_DERIVED(gelf_appender_args, (appender_args), (host_name)(additional_info))
FC_REFLECT(logger_args, (name)(level)(appender))

namespace logger {

fc::appender_config get_console_config(const std::string& s)
{
    auto appender = fc::json::from_string(s).as<appender_args>();

    auto stream = fc::variant(appender.stream).as<fc::console_appender::stream::type>();

    fc::console_appender::config config;
    config.level_colors.emplace_back(
        fc::console_appender::level_color(fc::log_level::debug, fc::console_appender::color::white));
    config.level_colors.emplace_back(
        fc::console_appender::level_color(fc::log_level::info, fc::console_appender::color::green));
    config.level_colors.emplace_back(
        fc::console_appender::level_color(fc::log_level::warn, fc::console_appender::color::brown));
    config.level_colors.emplace_back(
        fc::console_appender::level_color(fc::log_level::error, fc::console_appender::color::red));
    config.stream = stream;

    return fc::appender_config::create_config<fc::console_appender>(appender.appender, fc::variant(config));
}

fc::appender_config get_file_config(const std::string& s, const boost::filesystem::path& pwd)
{
    auto file_appender = fc::json::from_string(s).as<file_appender_args>();

    fc::path file_name = file_appender.stream;

    if (file_name.is_relative())
    {
        file_name = fc::absolute(pwd) / file_name;
    }

    ilog(file_name.generic_string());

    if (fc::is_directory(file_name))
    {
        FC_THROW_EXCEPTION(fc::invalid_arg_exception, "appender stream ${name} is not a file", ("name", file_name));
    }

    // construct a default file appender config here
    // filename will be taken from ini file, everything else hard-coded here
    fc::file_appender::config config;
    config.filename = file_name;
    config.flush = true;
    config.rotate = true;
    config.rotation_interval = fc::minutes(file_appender.rotation_interval_minutes);
    config.rotation_limit = fc::hours(file_appender.rotation_limit_hours);

    return fc::appender_config::create_config<fc::file_appender>(file_appender.appender, fc::variant(config));
}

fc::appender_config get_gelf_config(const std::string& s, const std::string& optional_info = "")
{
    auto gelf_appender = fc::json::from_string(s).as<gelf_appender_args>();

    // check if stream can be network endpoint
    std::string endpoint = gelf_appender.stream;
    if (endpoint.find(':') == std::string::npos)
    {
        endpoint += ":"; // add default port
        endpoint += boost::lexical_cast<std::string>(DEFAULT_GELF_APPENDER_PORT);
    }
    fc::ip::endpoint::from_string(endpoint); // if stream is not a network endpoint it will throw

    fc::gelf_appender::config config;
    config.endpoint = endpoint;
    config.host_name = gelf_appender.host_name;
    config.version = (fc::string)SCORUM_BLOCKCHAIN_VERSION;
    config.additional_info = optional_info;

    if (!gelf_appender.additional_info.empty())
    {
        if (!config.additional_info.empty())
            config.additional_info += ": ";
        config.additional_info += gelf_appender.additional_info;
    }

    return fc::appender_config::create_config<fc::gelf_appender>(gelf_appender.appender, fc::variant(config));
}

fc::optional<fc::logging_config> load_logging_config_from_options(const boost::program_options::variables_map& args,
                                                                  const boost::filesystem::path& pwd)
{
    // clang-format off
    try
    {
        fc::logging_config logging_config;
        bool found_logging_config = false;

        if (args.count(LOG_APPENDER))
        {
            std::vector<std::string> appenders = args[LOG_APPENDER].as<std::vector<std::string>>();

            for (const std::string& s : appenders)
            {
                //check if appender is a console appender
                try
                {
                    logging_config.appenders.push_back(get_console_config(s));

                    found_logging_config = true;
                    continue;
                }
                catch (fc::bad_cast_exception&)
                {
                }

                //check if appender is a gelf appender
                try
                {
                    //check if stream can be network endpoint
                    std::string witness;
                    if (args.count("witness"))
                    {
                        try
                        {
                            const std::vector<std::string>& witnesses = args["witness"].as<std::vector<std::string>>();
                            if (!witnesses.empty())
                            {
                                witness = "witness " + boost::algorithm::join(witnesses, " ");
                            }
                        }
                        FC_CAPTURE_AND_LOG(())
                    }

                    logging_config.appenders.push_back(get_gelf_config(s, witness));

                    found_logging_config = true;
                    continue;
                }
                catch (fc::exception& e)
                {
                }

                //check if appender is a file appender
                try
                {
                    logging_config.appenders.push_back(get_file_config(s, pwd));

                    found_logging_config = true;
                    continue;
                }
                catch (fc::exception&)
                {
                }
            }
        }

        if (args.count(LOGGER))
        {
            std::vector<std::string> loggers = args[LOGGER].as<std::vector<std::string>>();

            for (std::string& s : loggers)
            {
                auto logger = fc::json::from_string(s).as<logger_args>();

                fc::logger_config logger_config(logger.name);
                logger_config.level = fc::variant(logger.level).as<fc::log_level>();
                boost::split(logger_config.appenders, logger.appender, boost::is_any_of(" ,"), boost::token_compress_on);
                logging_config.loggers.push_back(logger_config);

                found_logging_config = true;
            }
        }

        if (found_logging_config)
            return logging_config;
        else
            return fc::optional<fc::logging_config>();
    }
    FC_RETHROW_EXCEPTIONS(warn, "")
    // clang-format on
}

void set_logging_program_options(boost::program_options::options_description& options)
{
    // clang-format off
    std::vector<std::string> default_appender({ 
        R"({"appender":"stderr","stream":"std_error"})",
        LOG_APPENDER R"( = {"appender":"p2p","stream":"logs/p2p.log","rotation_interval_minutes":"120", "rotation_limit_hours":"720"})",
        LOG_APPENDER R"( = {"appender":"node","stream":"logs/node.log","rotation_interval_minutes":"120", "rotation_limit_hours":"720"})",
        "# " LOG_APPENDER R"( = {"appender":"remote","stream":"127.0.0.1:12201", "host_name":"", "additional_info":""})"
        });
    std::string str_default_appender = boost::algorithm::join(default_appender, "\n");

   
    std::vector< std::string > default_logger(
        { R"({"name":"default","level":"info","appender":"stderr, node"})",
        LOGGER R"( = {"name":"p2p","level":"info","appender":"p2p"})" });
    std::string str_default_logger = boost::algorithm::join(default_logger, "\n");


    std::vector< std::string > default_appender_description(
                                { R"(Console appender definition json: {"appender", "stream"})" ,
                                  R"(File appender definition json:  {"appender", "stream", "rotation_interval_minutes", "rotation_limit_hours"})",
                                  R"(Gelf appender definition json:  {"appender", "stream", "host_name", "additional_info"})" });
    std::string str_default_appender_description = boost::algorithm::join(default_appender_description, "\n# ");

    auto default_value = [](const std::vector<std::string>& args, const std::string& str)
    {
        return boost::program_options::value<std::vector<std::string>>()->composing()->default_value(args, str);
    };

    options.add_options()
        (LOG_APPENDER, default_value(default_appender, str_default_appender), str_default_appender_description.c_str())
        (LOGGER, default_value(default_logger, str_default_logger), R"(Logger definition json: {"name", "level", "appender"})" );
    // clang-format on
}
}
