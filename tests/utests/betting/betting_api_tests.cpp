#include <boost/test/unit_test.hpp>

#include <scorum/app/betting_api_impl.hpp>

#include <hippomocks.h>

#include "object_wrapper.hpp"
#include "service_wrappers.hpp"

namespace betting_api_tests {

using namespace scorum::app;

class fixture : public shared_memory_fixture
{
public:
    MockRepository mocks;
    data_service_factory_i* factory = mocks.Mock<data_service_factory_i>();

    game_service_i* game_service = mocks.Mock<game_service_i>();
    bet_service_i* bet_service = mocks.Mock<bet_service_i>();
    pending_bet_service_i* pending_bet_service = mocks.Mock<pending_bet_service_i>();
    matched_bet_service_i* matched_bet_service = mocks.Mock<matched_bet_service_i>();

    fixture()
    {
    }

    void init()
    {
        mocks.OnCall(factory, data_service_factory_i::game_service).ReturnByRef(*game_service);
        mocks.OnCall(factory, data_service_factory_i::bet_service).ReturnByRef(*bet_service);
        mocks.OnCall(factory, data_service_factory_i::pending_bet_service).ReturnByRef(*pending_bet_service);
        mocks.OnCall(factory, data_service_factory_i::matched_bet_service).ReturnByRef(*matched_bet_service);
    }
};

BOOST_AUTO_TEST_SUITE(betting_api_tests)

BOOST_FIXTURE_TEST_CASE(get_services_in_constructor, fixture)
{
    game_service_i* game_service = mocks.Mock<game_service_i>();
    bet_service_i* bet_service = mocks.Mock<bet_service_i>();
    pending_bet_service_i* pending_bet_service = mocks.Mock<pending_bet_service_i>();
    matched_bet_service_i* matched_bet_service = mocks.Mock<matched_bet_service_i>();

    mocks.ExpectCall(factory, data_service_factory_i::game_service).ReturnByRef(*game_service);
    mocks.ExpectCall(factory, data_service_factory_i::bet_service).ReturnByRef(*bet_service);
    mocks.ExpectCall(factory, data_service_factory_i::pending_bet_service).ReturnByRef(*pending_bet_service);
    mocks.ExpectCall(factory, data_service_factory_i::matched_bet_service).ReturnByRef(*matched_bet_service);

    BOOST_REQUIRE_NO_THROW(betting_api_impl api(*factory));
}

BOOST_FIXTURE_TEST_CASE(get_games_dont_throw, fixture)
{
    init();

    betting_api_impl api(*factory);

    std::vector<game_object> objects;

    mocks.ExpectCall(game_service, game_service_i::get_games).Return({ objects.begin(), objects.end() });

    BOOST_REQUIRE_NO_THROW(api.get_games(game_filter::all));
}

struct get_games_fixture : public fixture
{
    get_games_fixture()
    {
        init();

        objects.push_back(
            create_object<game_object>(shm, [&](game_object& game) { game.status = game_status::created; }));

        objects.push_back(
            create_object<game_object>(shm, [&](game_object& game) { game.status = game_status::started; }));

        objects.push_back(
            create_object<game_object>(shm, [&](game_object& game) { game.status = game_status::finished; }));
    }

    std::vector<game_object> objects;
};

BOOST_FIXTURE_TEST_CASE(get_games_return_all_games, get_games_fixture)
{
    mocks.ExpectCall(game_service, game_service_i::get_games).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    std::vector<game_api_object> games = api.get_games(game_filter::all);

    BOOST_CHECK_EQUAL(games.size(), 3u);
}

BOOST_FIXTURE_TEST_CASE(get_games_does_not_change_order, get_games_fixture)
{
    mocks.ExpectCall(game_service, game_service_i::get_games).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    std::vector<game_api_object> games = api.get_games(game_filter::all);

    BOOST_CHECK(games[0].status == game_status::created);
    BOOST_CHECK(games[1].status == game_status::started);
    BOOST_CHECK(games[2].status == game_status::finished);
}

BOOST_FIXTURE_TEST_CASE(return_games_with_created_status, get_games_fixture)
{
    mocks.ExpectCall(game_service, game_service_i::get_games).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    std::vector<game_api_object> games = api.get_games(game_filter::created);

    BOOST_REQUIRE_EQUAL(games.size(), 1);
    BOOST_CHECK(games[0].status == game_status::created);
}

BOOST_FIXTURE_TEST_CASE(return_games_with_started_status, get_games_fixture)
{
    mocks.ExpectCall(game_service, game_service_i::get_games).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    std::vector<game_api_object> games = api.get_games(game_filter::started);

    BOOST_REQUIRE_EQUAL(games.size(), 1);
    BOOST_CHECK(games[0].status == game_status::started);
}

BOOST_FIXTURE_TEST_CASE(return_games_with_finished_status, get_games_fixture)
{
    mocks.ExpectCall(game_service, game_service_i::get_games).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    std::vector<game_api_object> games = api.get_games(game_filter::finished);

    BOOST_REQUIRE_EQUAL(games.size(), 1);
    BOOST_CHECK(games[0].status == game_status::finished);
}

BOOST_FIXTURE_TEST_CASE(return_two_games_with_finished_status, get_games_fixture)
{
    objects.push_back(create_object<game_object>(shm, [&](game_object& game) { game.status = game_status::finished; }));

    mocks.ExpectCall(game_service, game_service_i::get_games).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    std::vector<game_api_object> games = api.get_games(game_filter::finished);

    BOOST_REQUIRE_EQUAL(games.size(), 2);
    BOOST_CHECK(games[0].status == game_status::finished);
    BOOST_CHECK(games[1].status == game_status::finished);
}

BOOST_FIXTURE_TEST_CASE(return_games_not_finished_status, get_games_fixture)
{
    mocks.ExpectCall(game_service, game_service_i::get_games).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    std::vector<game_api_object> games = api.get_games(game_filter::not_finished);

    BOOST_REQUIRE_EQUAL(games.size(), 2);
    BOOST_CHECK(games[0].status == game_status::created);
    BOOST_CHECK(games[1].status == game_status::started);
}

BOOST_FIXTURE_TEST_CASE(throw_exception_when_limit_is_negative, get_games_fixture)
{
    betting_api_impl api(*factory);

    BOOST_REQUIRE_THROW(api.get_user_bets(0, -1), fc::assert_exception);
    BOOST_REQUIRE_THROW(api.get_pending_bets(0, -1), fc::assert_exception);
    BOOST_REQUIRE_THROW(api.get_matched_bets(0, -1), fc::assert_exception);
}

BOOST_FIXTURE_TEST_CASE(throw_exception_when_limit_gt_than_max_limit, get_games_fixture)
{
    const auto max_limit = 100;

    betting_api_impl api(*factory, max_limit);

    BOOST_REQUIRE_THROW(api.get_user_bets(0, max_limit + 1), fc::assert_exception);
    BOOST_REQUIRE_THROW(api.get_pending_bets(0, max_limit + 1), fc::assert_exception);
    BOOST_REQUIRE_THROW(api.get_matched_bets(0, max_limit + 1), fc::assert_exception);
}

BOOST_FIXTURE_TEST_CASE(dont_throw_when_limit_is_zero, get_games_fixture)
{
    betting_api_impl api(*factory);

    std::vector<bet_object> bets;
    std::vector<pending_bet_object> pbets;
    std::vector<matched_bet_object> mbets;

    mocks.OnCall(bet_service, bet_service_i::get_bets).With(_).ReturnByRef({ bets.begin(), bets.end() });

    mocks.OnCall(pending_bet_service, pending_bet_service_i::get_bets)
        .With(_)
        .ReturnByRef({ pbets.begin(), pbets.end() });

    mocks.OnCall(matched_bet_service, matched_bet_service_i::get_bets)
        .With(_)
        .ReturnByRef({ mbets.begin(), mbets.end() });

    BOOST_REQUIRE_NO_THROW(api.get_user_bets(0, 0));
    BOOST_REQUIRE_NO_THROW(api.get_pending_bets(0, 0));
    BOOST_REQUIRE_NO_THROW(api.get_matched_bets(0, 0));
}

BOOST_FIXTURE_TEST_CASE(dont_throw_when_limit_eq_max, get_games_fixture)
{
    const auto max_limit = 100;

    betting_api_impl api(*factory, max_limit);

    std::vector<bet_object> bets;
    std::vector<pending_bet_object> pbets;
    std::vector<matched_bet_object> mbets;

    mocks.OnCall(bet_service, bet_service_i::get_bets).With(_).ReturnByRef({ bets.begin(), bets.end() });

    mocks.OnCall(pending_bet_service, pending_bet_service_i::get_bets)
        .With(_)
        .ReturnByRef({ pbets.begin(), pbets.end() });

    mocks.OnCall(matched_bet_service, matched_bet_service_i::get_bets)
        .With(_)
        .ReturnByRef({ mbets.begin(), mbets.end() });

    BOOST_REQUIRE_NO_THROW(api.get_user_bets(0, max_limit));
    BOOST_REQUIRE_NO_THROW(api.get_pending_bets(0, max_limit));
    BOOST_REQUIRE_NO_THROW(api.get_matched_bets(0, max_limit));
}

template <typename T> struct get_bets_fixture : public fixture
{
    get_bets_fixture()
    {
        init();

        objects.push_back(create_object<T>(shm, [&](auto& bet) { bet.id = 0; }));
        objects.push_back(create_object<T>(shm, [&](auto& bet) { bet.id = 1; }));
        objects.push_back(create_object<T>(shm, [&](auto& bet) { bet.id = 2; }));
    }

    std::vector<T> objects;
};

BOOST_FIXTURE_TEST_CASE(check_get_bets_from_arg, get_bets_fixture<bet_object>)
{
    bet_id_type from = 0;
    mocks.ExpectCall(bet_service, bet_service_i::get_bets).With(from).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    api.get_user_bets(from, 1);
}

BOOST_FIXTURE_TEST_CASE(get_one_bet, get_bets_fixture<bet_object>)
{
    mocks.ExpectCall(bet_service, bet_service_i::get_bets).With(_).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    auto bets = api.get_user_bets(0, 1);

    BOOST_REQUIRE_EQUAL(bets.size(), 1);

    BOOST_CHECK(bets[0].id == 0u);
}

BOOST_FIXTURE_TEST_CASE(get_all_bets, get_bets_fixture<bet_object>)
{
    mocks.ExpectCall(bet_service, bet_service_i::get_bets).With(_).Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    auto bets = api.get_user_bets(0, 100);

    BOOST_REQUIRE_EQUAL(bets.size(), 3);

    BOOST_CHECK(bets[0].id == 0u);
    BOOST_CHECK(bets[1].id == 1u);
    BOOST_CHECK(bets[2].id == 2u);
}

BOOST_FIXTURE_TEST_CASE(check_get_pending_bets_from_arg, get_bets_fixture<pending_bet_object>)
{
    pending_bet_id_type from = 0;
    mocks.ExpectCall(pending_bet_service, pending_bet_service_i::get_bets)
        .With(from)
        .Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    api.get_pending_bets(from, 1);
}

BOOST_FIXTURE_TEST_CASE(get_one_pending_bet, get_bets_fixture<pending_bet_object>)
{
    mocks.ExpectCall(pending_bet_service, pending_bet_service_i::get_bets)
        .With(_)
        .Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    auto bets = api.get_pending_bets(0, 1);

    BOOST_REQUIRE_EQUAL(bets.size(), 1);

    BOOST_CHECK(bets[0].id == 0u);
}

BOOST_FIXTURE_TEST_CASE(get_all_pending_bets, get_bets_fixture<pending_bet_object>)
{
    mocks.ExpectCall(pending_bet_service, pending_bet_service_i::get_bets)
        .With(_)
        .Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    auto bets = api.get_pending_bets(0, 100);

    BOOST_REQUIRE_EQUAL(bets.size(), 3);

    BOOST_CHECK(bets[0].id == 0u);
    BOOST_CHECK(bets[1].id == 1u);
    BOOST_CHECK(bets[2].id == 2u);
}

BOOST_FIXTURE_TEST_CASE(check_get_matched_bets_from_arg, get_bets_fixture<matched_bet_object>)
{
    matched_bet_id_type from = 0;
    mocks.ExpectCall(matched_bet_service, matched_bet_service_i::get_bets)
        .With(from)
        .Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    api.get_matched_bets(from, 1);
}

BOOST_FIXTURE_TEST_CASE(get_one_matched_bet, get_bets_fixture<matched_bet_object>)
{
    mocks.ExpectCall(matched_bet_service, matched_bet_service_i::get_bets)
        .With(_)
        .Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    auto bets = api.get_matched_bets(0, 1);

    BOOST_REQUIRE_EQUAL(bets.size(), 1);

    BOOST_CHECK(bets[0].id == 0u);
}

BOOST_FIXTURE_TEST_CASE(get_all_matched_bets, get_bets_fixture<matched_bet_object>)
{
    mocks.ExpectCall(matched_bet_service, matched_bet_service_i::get_bets)
        .With(_)
        .Return({ objects.begin(), objects.end() });

    betting_api_impl api(*factory);
    auto bets = api.get_matched_bets(0, 100);

    BOOST_REQUIRE_EQUAL(bets.size(), 3);

    BOOST_CHECK(bets[0].id == 0u);
    BOOST_CHECK(bets[1].id == 1u);
    BOOST_CHECK(bets[2].id == 2u);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace betting_api_tests
