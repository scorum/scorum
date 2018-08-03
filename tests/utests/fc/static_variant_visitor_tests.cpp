#include <type_traits>
#include <boost/test/unit_test.hpp>
#include <fc/static_variant.hpp>
#include <fc/optional.hpp>
#include "defines.hpp"

namespace {
struct generic_visitor
{
    size_t operator()(const std::vector<int>& x) const
    {
        return x.size();
    }
    template <typename T> size_t operator()(const T& x) const
    {
        return x.size();
    }
};

BOOST_AUTO_TEST_SUITE(static_variant_visitor_tests)

SCORUM_TEST_CASE(generic_lambda_test)
{
    fc::static_variant<std::vector<int>, std::set<std::string>> var;

    BOOST_TEST_MESSAGE("Generic rvalue lambda with return value");
    {
        var = std::vector<int>{ 1, 2 };
        auto size = var.visit([](const auto& x) { return x.size(); });
        BOOST_REQUIRE_EQUAL(size, 2u);
    }
    BOOST_TEST_MESSAGE("Generic lvalue lambda with return value");
    {
        var = std::set<std::string>{ std::string{} };
        auto visitor = [](const auto& x) { return x.size(); };
        BOOST_REQUIRE_EQUAL(var.visit(visitor), 1u);
    }
    BOOST_TEST_MESSAGE("Generic rvalue lambda without return value");
    {
        var = std::set<std::string>{ std::string{} };
        size_t size = 0;
        var.visit([&](const auto& x) { size = x.size(); });
        BOOST_REQUIRE_EQUAL(size, 1u);
    }
    BOOST_TEST_MESSAGE("Accept variant's value by value");
    {
        var = std::set<std::string>{ std::string{} };
        auto size = var.visit([&](auto x) { return x.size(); });
        BOOST_REQUIRE_EQUAL(size, 1u);
    }
}

SCORUM_TEST_CASE(lambda_list_strict_visit_test)
{
    fc::static_variant<std::vector<int>, std::set<std::string>> var;

    BOOST_TEST_MESSAGE("Rvalue lambda list with return value");
    {
        var = std::vector<int>{ 1, 2 };
        auto size = var.visit([](const std::vector<int>& x) { return x.size(); },
                              [](const std::set<std::string>& x) { return x.size(); });
        BOOST_REQUIRE_EQUAL(size, 2u);
    }
    BOOST_TEST_MESSAGE("Lvalue lambda list with return value");
    {
        var = std::set<std::string>{ std::string{} };
        auto v1 = [](const std::vector<int>& x) { return x.size(); };
        auto v2 = [](const std::set<std::string>& x) { return x.size(); };
        BOOST_REQUIRE_EQUAL(var.visit(v1, v2), 1u);
    }
    BOOST_TEST_MESSAGE("Rvalue lambda list without return value");
    {
        size_t size = 0;
        var = std::vector<int>{ 1, 2 };
        var.visit([&](const std::vector<int>& x) { size = x.size(); },
                  [&](const std::set<std::string>& x) { size = x.size(); });
        BOOST_REQUIRE_EQUAL(size, 2u);
    }
    BOOST_TEST_MESSAGE("Accept variant's value by value");
    {
        size_t size = 0;
        var = std::vector<int>{ 1, 2 };
        var.visit([&](std::vector<int> x) { size = x.size(); }, [&](std::set<std::string> x) { size = x.size(); });
        BOOST_REQUIRE_EQUAL(size, 2u);
    }
    {
        /*
         * This one does not compile cuz not all variant's visitors are provided
         */
        // auto size = var.visit([&](const std::vector<int>& x) { return x.size(); });
    }
}

SCORUM_TEST_CASE(generic_lambda_weak_visit_test)
{
    /*
     * Actually it doesn't make sence to pass generic lambda into weak_visit.
     * You should use visit in such cases, cuz visit is more strict than weak_visit and requires all variants overloads
     *
     * This one doesn't compile cuz there is no possibility to deduce return type from templated method
     */
    // fc::static_variant<std::vector<int>, std::set<std::string>> var;
    // auto size = var.weak_visit([](const auto& x) { return x.size(); });
}

SCORUM_TEST_CASE(lambda_list_weak_visit_test)
{
    fc::static_variant<std::vector<int>, std::set<std::string>, std::map<int, int>> var;

    BOOST_TEST_MESSAGE("Rvalue lambda list with return value. Found required overload");
    {
        var = std::vector<int>{ 1, 2 };
        auto size = var.weak_visit([](const std::vector<int>& x) { return x.size(); });
        static_assert(std::is_same<decltype(size), fc::optional<size_t>>::value, "");
        BOOST_REQUIRE_EQUAL(size.value(), 2u);
    }
    BOOST_TEST_MESSAGE("Rvalue lambda list with return value. Didn't find required overload");
    {
        var = std::set<std::string>{ std::string{} };
        auto size = var.weak_visit([](const std::vector<int>& x) { return x.size(); },
                                   [](const std::map<int, int>& x) { return x.size(); });
        static_assert(std::is_same<decltype(size), fc::optional<size_t>>::value, "");
        BOOST_CHECK(!size.valid());
    }
    BOOST_TEST_MESSAGE("Lvalue lambda list with return value");
    {
        var = std::map<int, int>{ { 1, 2 } };
        auto v1 = [](const std::map<int, int>& x) { return x.begin(); };
        auto begin_it = var.weak_visit(v1);
        static_assert(std::is_same<decltype(begin_it), fc::optional<std::map<int, int>::const_iterator>>::value, "");
        BOOST_REQUIRE_EQUAL(begin_it.value()->first, (var.get<std::map<int, int>>().begin())->first);
    }
}

SCORUM_TEST_CASE(functor_visit_test)
{
    fc::static_variant<std::vector<int>, std::set<std::string>> var;

    BOOST_TEST_MESSAGE("Functor with return value");
    {
        struct visitor
        {
            size_t operator()(const std::vector<int>& x) const
            {
                return x.size();
            }
            size_t operator()(const std::set<std::string>& x) const
            {
                return x.size();
            }
        };

        var = std::vector<int>{ 1, 2 };
        BOOST_REQUIRE_EQUAL(var.visit(visitor{}), 2u);
    }
    BOOST_TEST_MESSAGE("Functor without return value");
    {
        struct visitor
        {
            void operator()(const std::vector<int>&) const
            {
            }
            void operator()(const std::set<std::string>&) const
            {
            }
        };

        var.visit(visitor{});
    }
    BOOST_TEST_MESSAGE("Lvalue lambda list with return value");
    {
        var = std::set<std::string>{ std::string{} };
        BOOST_REQUIRE_EQUAL(var.visit(generic_visitor{}), 1u);
    }
}

BOOST_AUTO_TEST_SUITE_END()
}