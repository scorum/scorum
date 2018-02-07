#include <cstdlib>
#include <iostream>
#include <boost/test/included/unit_test.hpp>

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[])
{
    std::srand(time(NULL));
    std::cout << "Random number generator seeded to " << time(NULL) << std::endl;
    return nullptr;
}
