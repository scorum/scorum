#pragma once

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>

#include <fc/log/logger_config.hpp>

namespace logger {
void write_default_logging_config_to_stream(std::ostream& out);
void set_logging_program_options(boost::program_options::options_description& options);
fc::optional<fc::logging_config> load_logging_config_from_options(const boost::program_options::variables_map& args);
}
