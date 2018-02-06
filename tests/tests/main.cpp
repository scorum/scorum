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
#include <boost/test/included/unit_test.hpp>

#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/reflect/variant.hpp>

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[])
{
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
    lg.level = fc::log_level::info;
    lg.appenders.push_back("stderr");
    log_conf.loggers.push_back(lg);
    fc::configure_logging(log_conf);
    return nullptr;
}
