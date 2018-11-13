#include <boost/test/unit_test.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::protocol;

BOOST_AUTO_TEST_SUITE(wincase_type_tests)

SCORUM_TEST_CASE(create_opposite_test)
{
    {
        wincase_type w = result_home::yes{};
        auto opposite = create_opposite(w);
        BOOST_CHECK_NO_THROW(opposite.get<result_home::no>());
    }
    {
        wincase_type w = result_draw::no{};
        auto opposite = create_opposite(w);
        BOOST_CHECK_NO_THROW(opposite.get<result_draw::yes>());
    }
    {
        wincase_type w = handicap::over{ -500 };
        auto opposite = create_opposite(w);
        BOOST_REQUIRE_NO_THROW(opposite.get<handicap::under>());
        BOOST_CHECK_EQUAL(opposite.get<handicap::under>().threshold, -500);
    }
    {
        wincase_type w = correct_score::no{ 5, 0 };
        auto opposite = create_opposite(w);
        BOOST_REQUIRE_NO_THROW(opposite.get<correct_score::yes>());
        BOOST_CHECK_EQUAL(opposite.get<correct_score::yes>().home, 5);
        BOOST_CHECK_EQUAL(opposite.get<correct_score::yes>().away, 0);
    }
    {
        wincase_type w = total::under{ 1500 };
        auto opposite = create_opposite(w);
        BOOST_REQUIRE_NO_THROW(opposite.get<total::over>());
        BOOST_CHECK_EQUAL(opposite.get<total::over>().threshold, 1500);
    }
    {
        auto opposite = create_opposite(result_home::yes{});
        BOOST_CHECK_NO_THROW(opposite.get<result_home::no>());
    }
}

SCORUM_TEST_CASE(match_wincases_test_positive)
{
    {
        wincase_type lhs = result_home::yes{};
        wincase_type rhs = result_home::no{};
        BOOST_CHECK(match_wincases(lhs, rhs));
    }
    {
        wincase_type lhs = result_draw::no{};
        wincase_type rhs = result_draw::yes{};
        BOOST_CHECK(match_wincases(lhs, rhs));
    }
    {
        wincase_type lhs = handicap::over{ -500 };
        wincase_type rhs = handicap::under{ -500 };
        BOOST_CHECK(match_wincases(lhs, rhs));
    }
    {
        BOOST_CHECK(match_wincases(correct_score_home::no{}, correct_score_home::yes{}));
    }
    {
        wincase_type lhs = correct_score::no{ 5, 0 };
        wincase_type rhs = correct_score::yes{ 5, 0 };
        BOOST_CHECK(match_wincases(lhs, rhs));
    }
}

SCORUM_TEST_CASE(match_wincases_test_negative)
{
    {
        wincase_type lhs = result_home::yes{};
        wincase_type rhs = result_draw::no{};
        BOOST_CHECK(!match_wincases(lhs, rhs));
    }
    {
        wincase_type lhs = result_home::yes{};
        wincase_type rhs = result_home::yes{};
        BOOST_CHECK(!match_wincases(lhs, rhs));
    }
    {
        wincase_type lhs = handicap::over{ -500 };
        wincase_type rhs = handicap::under{ 0 };
        BOOST_CHECK(!match_wincases(lhs, rhs));
    }
    {
        BOOST_CHECK(!match_wincases(correct_score::yes{ 1, 2 }, correct_score::yes{ 1, 2 }));
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
