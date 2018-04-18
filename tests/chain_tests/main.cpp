#include <cstdlib>
#include <iostream>
#include <boost/make_unique.hpp>
#include <boost/test/included/unit_test.hpp>

#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/reflect/variant.hpp>

#include <scorum/protocol/config.hpp>

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[])
{
    using namespace scorum::protocol;
    detail::override_config(boost::make_unique<detail::config>(detail::config::test));

    std::srand(time(NULL));
    std::cout << "Random number generator seeded to " << time(NULL) << std::endl;
    fc::logging_config log_conf;

    fc::variants c;
    c.push_back(fc::mutable_variant_object("level", "debug")("color", "green"));
    c.push_back(fc::mutable_variant_object("level", "warn")("color", "brown"));
    c.push_back(fc::mutable_variant_object("level", "error")("color", "red"));

    log_conf.appenders.push_back(fc::appender_config(
        "stderr", "console", fc::mutable_variant_object()("stream", "std_error")("level_colors", c)));
    log_conf.appenders.push_back(
        fc::appender_config("stdout", "console", fc::mutable_variant_object()("stream", "std_out")("level_colors", c)));

    fc::logger_config lg;
    lg.name = "default";
    lg.level = fc::log_level::warn;
    lg.appenders.push_back("stderr");
    log_conf.loggers.push_back(lg);
    fc::configure_logging(log_conf);
    return nullptr;
}
