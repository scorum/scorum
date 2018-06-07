#include <boost/test/unit_test.hpp>

#include <fc/filesystem.hpp>

#include <scorum/app/application.hpp>

using namespace scorum::app;

namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(app_tests)

BOOST_AUTO_TEST_CASE(throw_if_data_dir_is_not_set)
{
    boost::program_options::variables_map options;

    BOOST_REQUIRE_THROW(get_data_dir_path(options), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(do_not_throw_if_data_dir_set)
{
    boost::program_options::variables_map options;
    options.insert(std::make_pair("data-dir", bpo::variable_value(bfs::path("data_dir"), false)));

    bpo::notify(options);

    BOOST_REQUIRE_NO_THROW(get_data_dir_path(options));
}

BOOST_AUTO_TEST_CASE(return_absolute_path_if_data_dir_is_relative)
{
    boost::program_options::variables_map options;
    options.insert(std::make_pair("data-dir", bpo::variable_value(bfs::path("data_dir"), false)));

    bpo::notify(options);

    auto current_path = bfs::current_path();

    BOOST_REQUIRE_EQUAL(0, chdir("/tmp"));

    BOOST_CHECK_EQUAL(get_data_dir_path(options).generic_string(), "/tmp/data_dir");

    BOOST_REQUIRE_EQUAL(0, chdir(current_path.generic_string().c_str()));
}

BOOST_AUTO_TEST_CASE(do_not_modify_absolute_path)
{
    boost::program_options::variables_map options;
    options.insert(std::make_pair("data-dir", bpo::variable_value(bfs::path("/tmp/scorum/data_dir"), false)));

    bpo::notify(options);

    BOOST_CHECK_EQUAL(get_data_dir_path(options).generic_string(), "/tmp/scorum/data_dir");
}

BOOST_AUTO_TEST_CASE(get_config_file_path_based_on_absolute_data_dir_path)
{
    boost::program_options::variables_map options;
    options.insert(std::make_pair("data-dir", bpo::variable_value(bfs::path("/tmp/scorum/data_dir"), false)));

    bpo::notify(options);

    BOOST_CHECK_EQUAL(get_config_file_path(options).generic_string(), "/tmp/scorum/data_dir/config.ini");
}

BOOST_AUTO_TEST_CASE(get_config_file_path_based_on_relative_data_dir_path)
{
    boost::program_options::variables_map options;
    options.insert(std::make_pair("data-dir", bpo::variable_value(bfs::path("data_dir"), false)));

    bpo::notify(options);

    auto current_path = bfs::current_path();

    BOOST_REQUIRE_EQUAL(0, chdir("/tmp"));

    BOOST_CHECK_EQUAL(get_config_file_path(options).generic_string(), "/tmp/data_dir/config.ini");

    BOOST_REQUIRE_EQUAL(0, chdir(current_path.generic_string().c_str()));
}

BOOST_AUTO_TEST_CASE(create_config_file)
{
    fc::temp_file file;

    boost::program_options::options_description opt;

    BOOST_REQUIRE(!fc::exists(file.path()));

    BOOST_REQUIRE_NO_THROW(create_config_file_if_not_exist(file.path(), opt));

    BOOST_CHECK(fc::exists(file.path()));
}

BOOST_AUTO_TEST_CASE(throw_exception_when_path_is_not_absolute)
{
    boost::program_options::options_description opt;
    BOOST_REQUIRE_THROW(create_config_file_if_not_exist("config.ini", opt), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()
