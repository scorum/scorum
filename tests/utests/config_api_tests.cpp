#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/common_api/config_api.hpp>

#include <fc/exception/exception.hpp>

namespace config_api_tests {

using namespace scorum;

namespace po = boost::program_options;

struct config_api_fixture
{
    config_api_fixture()
    {
        BOOST_REQUIRE_NO_THROW(desc.add(get_api_config().get_options_descriptions()));
        BOOST_REQUIRE_NO_THROW(desc.add(get_api_config(any_api_name).get_options_descriptions()));
    }

    po::variables_map parse_input(const std::vector<std::string>& input)
    {
        try
        {
            // Parse mocked up input.
            po::variables_map vm;
            po::store(po::command_line_parser(input).options(desc).run(), vm);
            po::notify(vm);
            return vm;
        }
        FC_LOG_AND_RETHROW()
    }

    po::options_description desc;

    const std::string any_api_name = "any_api";
};

BOOST_FIXTURE_TEST_SUITE(config_api_tests, config_api_fixture)

BOOST_AUTO_TEST_CASE(get_api_config_no_throw)
{
    BOOST_REQUIRE_NO_THROW(get_api_config().lookup_limit);

    BOOST_REQUIRE_NO_THROW(get_api_config(any_api_name).lookup_limit);
}

BOOST_AUTO_TEST_CASE(set_and_reset_options_for_api_config)
{
    BOOST_CHECK_EQUAL(get_api_config().lookup_limit, 1000u); // default
    BOOST_CHECK_EQUAL(get_api_config(any_api_name).lookup_limit, 1000u); // default
    BOOST_CHECK_EQUAL(get_api_config().tags_to_analize_count, 8u); // default
    BOOST_CHECK_EQUAL(get_api_config(any_api_name).tags_to_analize_count, 8u); // default

    std::vector<std::string> input = {
        R"(--any-api-lookup-limit=222000)"
    };

    BOOST_REQUIRE_NO_THROW(get_api_config().set_options(parse_input(input)));
    BOOST_REQUIRE_NO_THROW(get_api_config(any_api_name).set_options(parse_input(input)));

    BOOST_CHECK_EQUAL(get_api_config().lookup_limit, 1000u); // default
    BOOST_CHECK_EQUAL(get_api_config(any_api_name).lookup_limit, 222000u);
    BOOST_CHECK_EQUAL(get_api_config().tags_to_analize_count, 8u); // default
    BOOST_CHECK_EQUAL(get_api_config(any_api_name).tags_to_analize_count, 8u); // default

    input = { R"(--any-api-tags-to-analize-count=11)" };

    BOOST_REQUIRE_NO_THROW(get_api_config().set_options(parse_input(input)));
    BOOST_REQUIRE_NO_THROW(get_api_config(any_api_name).set_options(parse_input(input)));

    BOOST_CHECK_EQUAL(get_api_config().lookup_limit, 1000u); // default
    BOOST_CHECK_EQUAL(get_api_config(any_api_name).lookup_limit, 1000u); // default
    BOOST_CHECK_EQUAL(get_api_config().tags_to_analize_count, 8u); // default
    BOOST_CHECK_EQUAL(get_api_config(any_api_name).tags_to_analize_count, 11u);
}

BOOST_AUTO_TEST_CASE(options_priority_check)
{
    std::vector<std::string> input = {
        R"(--api-lookup-limit=333000)", R"(--api-tags-to-analize-count=11)"
    };

    BOOST_REQUIRE_NO_THROW(get_api_config().set_options(parse_input(input)));
    BOOST_REQUIRE_NO_THROW(get_api_config(any_api_name).set_options(parse_input(input)));

    BOOST_CHECK_EQUAL(get_api_config().lookup_limit, 333000u);
    BOOST_CHECK_EQUAL(get_api_config(any_api_name).lookup_limit, 333000u); // take from root
    BOOST_CHECK_EQUAL(get_api_config().tags_to_analize_count, 11u);
    BOOST_CHECK_EQUAL(get_api_config(any_api_name).tags_to_analize_count, 11u); // take from root
}

BOOST_AUTO_TEST_SUITE_END()
}
