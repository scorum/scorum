#include <boost/test/unit_test.hpp>

#include <scorum/protocol/betting/wincase_comparison.hpp>

#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

BOOST_AUTO_TEST_SUITE(wincase_comparison_tests)

SCORUM_TEST_CASE(wincase_less_operator_tests)
{
    {
        correct_score_home_yes wl;
        correct_score_home_yes wr;

        BOOST_CHECK(!(wl < wr));
        BOOST_CHECK(!(wr < wl));
    }
    {
        handicap_home_over wl{ { 1000 } };
        handicap_home_over wr{ { 1500 } };

        BOOST_CHECK((wl < wr));
        BOOST_CHECK(!(wr < wl));
    }
    {
        handicap_home_over wl{ { -1000 } };
        handicap_home_over wr{ { -1000 } };

        BOOST_CHECK(!(wl < wr));
        BOOST_CHECK(!(wr < wl));
    }
    {
        correct_score_yes wl{ 1, 3 };
        correct_score_yes wr{ 1, 2 };

        BOOST_CHECK(!(wl < wr));
        BOOST_CHECK((wr < wl));
    }
    {
        correct_score_yes wl{ 1, 2 };
        correct_score_yes wr{ 1, 2 };

        BOOST_CHECK(!(wl < wr));
        BOOST_CHECK(!(wr < wl));
    }
}

SCORUM_TEST_CASE(wincase_less_functor_tests)
{
    std::less<wincase_type> less;
    {
        wincase_type wl = correct_score_home_yes{};
        wincase_type wr = correct_score_home_no{};

        std::less<wincase_type> less;

        BOOST_CHECK(less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        wincase_type wl = correct_score_home_yes{};
        correct_score_home_no wr;

        BOOST_CHECK(less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        correct_score_home_yes wl;
        wincase_type wr = correct_score_home_no{};

        BOOST_CHECK(less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        wincase_type wl = correct_score_home_no{};
        wincase_type wr = correct_score_home_yes{};

        BOOST_CHECK(!less(wl, wr));
        BOOST_CHECK(less(wr, wl));
    }
    {
        wincase_type wl = correct_score_home_yes{};
        wincase_type wr = correct_score_home_yes{};

        BOOST_CHECK(!less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        wincase_type wl = correct_score_home_yes{};
        correct_score_home_yes wr;

        BOOST_CHECK(!less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
    {
        correct_score_home_yes wl;
        wincase_type wr = correct_score_home_yes{};

        BOOST_CHECK(!less(wl, wr));
        BOOST_CHECK(!less(wr, wl));
    }
}
SCORUM_TEST_CASE(wincase_pair_cmp_tests)
{
    {
        wincase_pair p1 = { correct_score_home_yes{}, correct_score_home_no{} };
        wincase_pair p2 = { correct_score_home_no{}, correct_score_home_yes{} };

        std::equal_to<wincase_pair> eq_cmp{};
        std::less<wincase_pair> less_cmp{};

        BOOST_CHECK(eq_cmp(p1, p2));
        BOOST_CHECK(!less_cmp(p1, p2));
        BOOST_CHECK(!less_cmp(p2, p1));
    }
    {
        wincase_pair p1 = { correct_score_yes{ 1, 1 }, correct_score_no{ 1, 1 } };
        wincase_pair p2 = { correct_score_no{ 1, 0 }, correct_score_yes{ 1, 0 } };

        std::equal_to<wincase_pair> eq_cmp{};
        std::less<wincase_pair> less_cmp{};

        BOOST_CHECK(!eq_cmp(p1, p2));
        BOOST_CHECK(!less_cmp(p1, p2));
        BOOST_CHECK(less_cmp(p2, p1));
    }
}
BOOST_AUTO_TEST_SUITE_END()
}