#include <cstdlib>
#include <iostream>
#include <boost/make_unique.hpp>
#include <boost/test/included/unit_test.hpp>

#include <fc/reflect/variant.hpp>

#include <scorum/protocol/config.hpp>

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[])
{
    using namespace scorum::protocol;
    detail::override_config(boost::make_unique<detail::config>(detail::config::test));

    std::srand(time(NULL));
    std::cout << "Random number generator seeded to " << time(NULL) << std::endl;

    return nullptr;
}
