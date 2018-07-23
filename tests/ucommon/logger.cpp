#include "logger.hpp"

#include <fc/log/console_appender.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/reflect/variant.hpp>

namespace tests {
void initialize_logger(const fc::log_level::values& lv)
{
    fc::logging_config cfg;

    cfg.loggers = { fc::logger_config("default") };
    cfg.loggers.front().level = lv;
    cfg.loggers.front().appenders = { "default" };

    cfg.appenders.push_back(fc::appender_config::create_config<fc::console_appender>(
        "default", fc::variant(fc::console_appender::config())));

    cfg.configure_logging(cfg);
}
}
