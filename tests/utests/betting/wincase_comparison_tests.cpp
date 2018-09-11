#include <boost/test/unit_test.hpp>

#include <scorum/protocol/betting/market.hpp>

#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::protocol;

BOOST_AUTO_TEST_SUITE(wincase_comparison_tests)

SCORUM_TEST_CASE(wincase_less_operator_tests)
{
    {
        correct_score_home::yes wl;
        correct_score_home::yes wr;

        BOOST_CHECK(!(wl < wr));
        BOOST_CHECK(!(wr < wl));
    }
    {
        handicap::over wl{ 1000 };
        handicap::over wr{ 1500 };

        BOOST_CHECK((wl < wr));
        BOOST_CHECK(!(wr < wl));
    }
    {
        handicap::over wl{ -1000 };
        handicap::over wr{ -1000 };

        BOOST_CHECK(!(wl < wr));
        BOOST_CHECK(!(wr < wl));
    }
    {
        correct_score::yes wl{ 1, 3 };
        correct_score::yes wr{ 1, 2 };

        BOOST_CHECK(!(wl < wr));
        BOOST_CHECK((wr < wl));
    }
    {
        correct_score::yes wl{ 1, 2 };
        correct_score::yes wr{ 1, 2 };

        BOOST_CHECK(!(wl < wr));
        BOOST_CHECK(!(wr < wl));
    }
}

SCORUM_TEST_CASE(wincase_less_functor_tests)
{
    std::less<wincase_type> less;
    {
        wincase_type wl = correct_score_home::yes{};
        wincase_type wr = correct_score_home::no{};

        std::less<wincase_type> less;

        BOOST_CHECK(less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        wincase_type wl = correct_score_home::yes{};
        correct_score_home::no wr;

        BOOST_CHECK(less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        correct_score_home::yes wl;
        wincase_type wr = correct_score_home::no{};

        BOOST_CHECK(less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        wincase_type wl = correct_score_home::no{};
        wincase_type wr = correct_score_home::yes{};

        BOOST_CHECK(!less(wl, wr));
        BOOST_CHECK(less(wr, wl));
    }
    {
        wincase_type wl = correct_score_home::yes{};
        wincase_type wr = correct_score_home::yes{};

        BOOST_CHECK(!less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        wincase_type wl = correct_score_home::yes{};
        correct_score_home::yes wr;

        BOOST_CHECK(!less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        correct_score_home::yes wl;
        wincase_type wr = correct_score_home::yes{};

        BOOST_CHECK(!less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
}
BOOST_AUTO_TEST_SUITE_END()
}
