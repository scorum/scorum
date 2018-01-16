#include <boost/test/unit_test.hpp>

#include <scorum/wallet/printer.hpp>

//#define PRINT_OUTPUT

#ifdef PRINT_OUTPUT
#include <iostream>
#endif

struct printer_tests_fixture
{
    scorum::wallet::printer p;
};

BOOST_FIXTURE_TEST_SUITE(printer_tests, printer_tests_fixture)

BOOST_AUTO_TEST_CASE(check_print_sequence)
{
    p.print_sequence(1, 2, 3, '4', "56");

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_CHECK_EQUAL(p.str(), "123456");
}

BOOST_AUTO_TEST_CASE(check_print_sequence2str)
{
    BOOST_CHECK_EQUAL(p.print_sequence2str(1, 2, 3, '4', "56"), "123456");
}

BOOST_AUTO_TEST_CASE(check_print_clear)
{
    p.print_raw("Atyyy38432749423323");
    p.print_field("Name: ", "Andrew");

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_REQUIRE(!p.str().empty());

    p.clear();

    BOOST_CHECK(p.str().empty());
}

BOOST_AUTO_TEST_CASE(check_print_field)
{
    p.print_field("Name: ", "Andrew");

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    //( + 1) - plus '\n'
    BOOST_CHECK_EQUAL(p.str().size(), p.screen_w + 1);

    p.clear();

    p.print_field("Score: ", 100);

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_CHECK_EQUAL(p.str().size(), p.screen_w + 1);
}

BOOST_AUTO_TEST_CASE(check_print_line)
{
    p.print_line('=');

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_CHECK_EQUAL(p.str().size(), p.screen_w + 1);
}

BOOST_AUTO_TEST_SUITE_END()

struct printer_cell_tests_fixture : public printer_tests_fixture
{
    int ws[5] = { 5, 10, 15, 20, 50 };
    const char fill_char = '*';

    int count() const
    {
        return sizeof(ws) / sizeof(int);
    }

    int width() const
    {
        int ret = 0;
        for (int w : ws)
        {
            ret += w;
        }
        return ret;
    }

    void print_cell(int ci, std::string data = "")
    {
        if (ci < count())
        {
            if (!data.empty())
                p.print_cell(data, ws[ci], count());
            else
                p.print_cell(std::string(ws[ci] - 1, fill_char), ws[ci], count());
        }
    }

    void print()
    {
        for (int ci = 0; ci < count(); ++ci)
        {
            print_cell(ci);
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(printer_cell_tests, printer_cell_tests_fixture)

BOOST_AUTO_TEST_CASE(check_print_cell)
{
    BOOST_REQUIRE_EQUAL((size_t)width(), p.screen_w);

    print();

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_REQUIRE_EQUAL(p.str().size(), p.screen_w);

    p.clear();

    print_cell(0, std::string(ws[0] + 1, fill_char));
    print_cell(1);
    print_cell(2, std::string(ws[0] - 2, fill_char));
    print_cell(3);
    print_cell(4);

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_REQUIRE_EQUAL(p.str().size(), p.screen_w);

    p.clear();

    print_cell(0, std::string(ws[0] + 1, fill_char));
    print_cell(1, std::string(ws[0] + 3, fill_char));
    print_cell(2);
    print_cell(3);
    print_cell(4, std::string(ws[0] - 2, fill_char));

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_REQUIRE_EQUAL(p.str().size(), p.screen_w);
}

BOOST_AUTO_TEST_CASE(check_print_cell_wrap)
{
    BOOST_REQUIRE_EQUAL((size_t)width(), p.screen_w);
    BOOST_REQUIRE_NE(std::string(1, fill_char), p.wrap_symbol);

    for (int ci = 0; ci < count(); ++ci)
    {
        print_cell(ci, std::string(ws[ci], fill_char));
    }

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_REQUIRE_NE(p.str().find(p.wrap_symbol), std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

struct printer_table_tests_fixture : public printer_cell_tests_fixture
{
    void print_table()
    {
        p.create_table(ws[0], ws[1], ws[2], ws[3], ws[4]);
        for (int ci = 0; ci < count(); ++ci)
        {
            p.print_cell(std::string(ws[ci] - 1, fill_char));
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(printer_table_tests, printer_table_tests_fixture)

BOOST_AUTO_TEST_CASE(check_create_table)
{
    BOOST_REQUIRE_NO_THROW(p.create_table(ws[0], ws[1], ws[2], ws[3], ws[4]));
}

BOOST_AUTO_TEST_CASE(check_print_table)
{
    BOOST_REQUIRE(p.create_table(ws[0], ws[1], ws[2], ws[3], ws[4]));

    BOOST_REQUIRE_EQUAL((size_t)width(), p.screen_w);

    print_table();

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_REQUIRE_EQUAL(p.str().size(), p.screen_w);
}

BOOST_AUTO_TEST_CASE(check_print_table_with_cells)
{
    BOOST_REQUIRE(p.create_table(ws[0], ws[1], ws[2], ws[3], ws[4]));

    BOOST_REQUIRE_EQUAL((size_t)width(), p.screen_w);

    print();

    p.print_endl();

    print_table();

    print();

#ifdef PRINT_OUTPUT
    std::cout << p.str() << std::endl;
#endif

    BOOST_REQUIRE_EQUAL(p.str().size(), p.screen_w * 3 + 2);
}

BOOST_AUTO_TEST_SUITE_END()
