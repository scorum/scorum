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

#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/bind.hpp>
#include <algorithm>

#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/gelf_appender.hpp>

#include <scorum/app/application.hpp>
#include <scorum/app/log_configurator.hpp>
#include <scorum/protocol/config.hpp>

#include <fc/log/console_appender.hpp>
#include <fc/log/file_appender.hpp>
#include <fc/log/gelf_appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#define LOG_APPENDER "log-appender"
#define LOGGER "log-logger"
#define DEFAULT_GELF_APPENDER_PORT 12201

namespace po = boost::program_options;
using scorum::protocol::version;

struct logger_config_fixture
{
    po::options_description desc;

    logger_config_fixture()
    {
        desc.add_options()(LOG_APPENDER, po::value<std::vector<std::string>>(),
                           "appender")(LOGGER, po::value<std::vector<std::string>>(), "logger");
    }

    po::variables_map parse_input(const std::vector<std::string>& input)
    {
        // Parse mocked up input.
        po::variables_map vm;
        po::store(po::command_line_parser(input).options(desc).run(), vm);
        po::notify(vm);
        return vm;
    }
};

BOOST_AUTO_TEST_SUITE(logger_tests)

BOOST_FIXTURE_TEST_CASE(check_appender_config_do_not_load_empty_stream, logger_config_fixture)
{
    // Mock up input.
    std::vector<std::string> input = {
        R"(--log-appender={"appender":"remote","stream":""})"
    };

    BOOST_TEST_MESSAGE("Parse commandline");
    auto vm = parse_input(input);

    BOOST_TEST_MESSAGE("load_logging_config_from_option");

    fc::optional<fc::logging_config> config = logger::load_logging_config_from_options(vm, boost::filesystem::path());

    BOOST_REQUIRE(!config.valid());
}

BOOST_FIXTURE_TEST_CASE(check_console_config_loading, logger_config_fixture)
{
    // Mock up input.
    std::vector<std::string> input = { R"(--log-appender={"appender":"stderr","stream":"std_error"})",
                                       R"(--log-appender={"appender":"stdout","stream":"std_out"})" };

    BOOST_TEST_MESSAGE("Parse commandline");
    auto vm = parse_input(input);

    BOOST_TEST_MESSAGE("load_logging_config_from_option");

    fc::optional<fc::logging_config> config = logger::load_logging_config_from_options(vm, boost::filesystem::path());

    BOOST_REQUIRE(config.valid());
    BOOST_REQUIRE_EQUAL(config->loggers.size(), 0u);
    BOOST_REQUIRE_EQUAL(config->appenders.size(), 2u);
    BOOST_REQUIRE_EQUAL(config->appenders[0].name, "stderr");
    BOOST_REQUIRE_EQUAL(config->appenders[1].name, "stdout");

    auto args1 = config->appenders[0].args.as<fc::console_appender::config>();
    auto args2 = config->appenders[1].args.as<fc::console_appender::config>();

    BOOST_REQUIRE_EQUAL(args1.stream, fc::console_appender::stream::std_error);
    BOOST_REQUIRE_EQUAL(args2.stream, fc::console_appender::stream::std_out);
}

BOOST_FIXTURE_TEST_CASE(check_gelf_config_loading, logger_config_fixture)
{
    // Mock up input.
    std::vector<std::string> input = {
        R"(--log-appender={"appender":"remote","stream":"127.0.0.1:1111", "host_name":"my_host", "additional_info":"putin"})"
    };

    BOOST_TEST_MESSAGE("Parse commandline");
    auto vm = parse_input(input);

    BOOST_TEST_MESSAGE("load_logging_config_from_option");

    fc::optional<fc::logging_config> config = logger::load_logging_config_from_options(vm, boost::filesystem::path());

    BOOST_REQUIRE(config.valid());
    BOOST_REQUIRE_EQUAL(config->loggers.size(), 0u);
    BOOST_REQUIRE_EQUAL(config->appenders.size(), 1u);
    BOOST_REQUIRE_EQUAL(config->appenders[0].name, "remote");

    auto args = config->appenders[0].args.as<fc::gelf_appender::config>();

    BOOST_REQUIRE_EQUAL(args.endpoint, "127.0.0.1:1111");
    BOOST_REQUIRE_EQUAL(args.host_name, "my_host");
    BOOST_REQUIRE_EQUAL(args.additional_info, "putin");
    BOOST_REQUIRE_EQUAL(args.version, (fc::string)SCORUM_BLOCKCHAIN_VERSION);
}

BOOST_FIXTURE_TEST_CASE(check_gelf_config_loading_with_default_port, logger_config_fixture)
{
    // Mock up input.
    std::vector<std::string> input = {
        R"(--log-appender={"appender":"remote","stream":"127.0.0.1"})"
    };

    BOOST_TEST_MESSAGE("Parse commandline");
    auto vm = parse_input(input);

    BOOST_TEST_MESSAGE("load_logging_config_from_option");

    fc::optional<fc::logging_config> config = logger::load_logging_config_from_options(vm, boost::filesystem::path());

    BOOST_REQUIRE(config.valid());
    BOOST_REQUIRE_EQUAL(config->loggers.size(), 0u);
    BOOST_REQUIRE_EQUAL(config->appenders.size(), 1u);
    BOOST_REQUIRE_EQUAL(config->appenders[0].name, "remote");

    auto args = config->appenders[0].args.as<fc::gelf_appender::config>();

    BOOST_REQUIRE_EQUAL(args.endpoint, "127.0.0.1:" + std::to_string(DEFAULT_GELF_APPENDER_PORT));
    BOOST_REQUIRE_EQUAL(args.host_name, "");
    BOOST_REQUIRE_EQUAL(args.additional_info, "");
    BOOST_REQUIRE_EQUAL(args.version, (fc::string)SCORUM_BLOCKCHAIN_VERSION);
}

BOOST_FIXTURE_TEST_CASE(check_file_config_loading, logger_config_fixture)
{
    // Mock up input.
    std::vector<std::string> input = {
        R"(--log-appender={"appender":"p2p","stream":"logs/p2p.log","rotation_interval_minutes":"3", "rotation_limit_hours":"7"})"
    };

    BOOST_TEST_MESSAGE("Parse commandline");
    auto vm = parse_input(input);

    BOOST_TEST_MESSAGE("load_logging_config_from_option");

    auto path = boost::filesystem::current_path();
    fc::optional<fc::logging_config> config = logger::load_logging_config_from_options(vm, path);

    BOOST_REQUIRE(config.valid());
    BOOST_REQUIRE_EQUAL(config->loggers.size(), 0u);
    BOOST_REQUIRE_EQUAL(config->appenders.size(), 1u);
    BOOST_REQUIRE_EQUAL(config->appenders[0].name, "p2p");

    auto args = config->appenders[0].args.as<fc::file_appender::config>();

    BOOST_REQUIRE_EQUAL(args.filename.string(), (fc::absolute(path) / "logs/p2p.log").string());
    BOOST_REQUIRE_EQUAL(args.flush, true);
    BOOST_REQUIRE_EQUAL(args.rotate, true);
    BOOST_REQUIRE_EQUAL(args.rotation_interval.count(), fc::minutes(3).count());
    BOOST_REQUIRE_EQUAL(args.rotation_limit.count(), fc::hours(7).count());
}

BOOST_FIXTURE_TEST_CASE(check_logger_config_loading, logger_config_fixture)
{
    // Mock up input.
    std::vector<std::string> input = {
        R"(--log-logger={"name":"default","level":"info","appender":"stderr, node"})"
    };

    BOOST_TEST_MESSAGE("Parse commandline");
    auto vm = parse_input(input);

    BOOST_TEST_MESSAGE("load_logging_config_from_option");

    fc::optional<fc::logging_config> config = logger::load_logging_config_from_options(vm, boost::filesystem::path());

    BOOST_REQUIRE(config.valid());
    BOOST_REQUIRE_EQUAL(config->loggers.size(), 1u);
    BOOST_REQUIRE_EQUAL(config->appenders.size(), 0u);

    BOOST_REQUIRE_EQUAL(config->loggers[0].name, "default");
    BOOST_REQUIRE_EQUAL(config->loggers[0].level, fc::log_level::info);
    BOOST_REQUIRE_EQUAL(config->loggers[0].appenders.size(), 2u);
    BOOST_REQUIRE_EQUAL(config->loggers[0].appenders[0], "stderr");
    BOOST_REQUIRE_EQUAL(config->loggers[0].appenders[1], "node");
}

BOOST_AUTO_TEST_SUITE_END()
