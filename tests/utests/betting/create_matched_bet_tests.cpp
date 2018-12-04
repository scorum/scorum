#include <boost/test/unit_test.hpp>

#include <boost/lexical_cast.hpp>

#include <scorum/utils/collect_range_adaptor.hpp>

#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/betting/betting_matcher.hpp>
#include <scorum/chain/betting/betting_math.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

#include "defines.hpp"
#include "db_mock.hpp"
#include "detail.hpp"

namespace create_matched_bet_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

using namespace scorum::utils::adaptors;

struct fixture
{
    fixture()
        : matched_dba(db)
        , _counter(0)
    {
        setup_db();
    }

    void setup_db()
    {
        db.add_index<pending_bet_index>();
        db.add_index<matched_bet_index>();
    }

    template <typename C> const pending_bet_object& create_bet(C&& constructor)
    {
        ++_counter;

        return db.create<pending_bet_object>([&](pending_bet_object& bet) {
            bet.data.uuid = gen_uuid(boost::lexical_cast<std::string>(_counter));
            bet.data.wincase = total::over({ 1 });

            constructor(bet);
        });
    }

    db_mock db;
    dba::db_accessor<matched_bet_object> matched_dba;
    matched_stake_type matched;

private:
    size_t _counter;
};

BOOST_FIXTURE_TEST_SUITE(pending_bet_index_by_game_uuid_wincase_tests, fixture)

template <typename Wincase>
auto get_range(const dba::db_accessor<pending_bet_object>& accessor, scorum::uuid_type id, Wincase w)
{
    auto key = std::make_tuple(id, w);
    return accessor.get_range_by<by_game_uuid_wincase_asc>(key);
}

SCORUM_TEST_CASE(check_order_in_range_of_bets_with_equal_game_id_and_wincase)
{
    dba::db_accessor<pending_bet_object> pending_dba(db);

    auto bet1 = create_bet([](pending_bet_object& bet) {
        bet.game_uuid = { 1 };
        bet.data.wincase = total::over({ 1 });
    });

    auto bet2 = create_bet([&](pending_bet_object& bet) {
        bet.game_uuid = { 1 };
        bet.data.wincase = total::over({ 1 });
    });

    auto range = get_range(pending_dba, bet1.game_uuid, total::over({ 1 }));

    auto pending_bets = range | collect<std::vector>();

    BOOST_REQUIRE_EQUAL(2u, pending_bets.size());

    BOOST_CHECK(pending_bets[0].data.uuid == bet1.data.uuid);
    BOOST_CHECK(pending_bets[1].data.uuid == bet2.data.uuid);
}

SCORUM_TEST_CASE(dont_return_bet_if_game_id_is_different)
{
    dba::db_accessor<pending_bet_object> pending_dba(db);

    auto bet1 = create_bet([](pending_bet_object& bet) {
        bet.game_uuid = { 1 };
        bet.data.wincase = total::over({ 1 });
    });

    create_bet([&](pending_bet_object& bet) {
        bet.game_uuid = { 2 };
        bet.data.wincase = total::over({ 1 });
    });

    auto range = get_range(pending_dba, bet1.game_uuid, total::over({ 1 }));

    BOOST_CHECK_EQUAL(1u, boost::distance(range));
    BOOST_CHECK(scorum::uuid_type{ 1 } == range.front().game_uuid);
}

SCORUM_TEST_CASE(dont_return_bet_if_wincase_is_different)
{
    dba::db_accessor<pending_bet_object> pending_dba(db);

    auto bet1 = create_bet([](pending_bet_object& bet) {
        bet.game_uuid = { 1 };
        bet.data.wincase = total::over({ 1 });
    });

    create_bet([&](pending_bet_object& bet) {
        bet.game_uuid = { 1 };
        bet.data.wincase = total::over({ 2 });
    });

    auto range = get_range(pending_dba, bet1.game_uuid, total::over({ 1 }));

    BOOST_CHECK_EQUAL(1u, boost::distance(range));

    BOOST_CHECK(total::over({ 1 }) == range.front().data.wincase);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(create_matched_bet_tests, fixture)

SCORUM_TEST_CASE(check_that_bet_created_with_valid_date)
{
    const auto created = fc::time_point_sec::from_iso_string("2018-01-01T00:00:00");

    auto bet1 = create_bet([](auto&) {});
    auto bet2 = create_bet([&](auto& bet) { bet.data.wincase = create_opposite(bet1.data.wincase); });

    create_matched_bet(matched_dba, bet1, bet2, matched, created);

    BOOST_CHECK(matched_dba.get().created == created);
}

SCORUM_TEST_CASE(matched_bet_market_equal_to_bet1_market)
{
    auto bet1 = create_bet([](auto& bet) { bet.market = total({ 1 }); });
    auto bet2 = create_bet([&](auto& bet) { bet.data.wincase = create_opposite(bet1.data.wincase); });

    create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec());

    BOOST_CHECK(matched_dba.get().market == bet1.market);
}

SCORUM_TEST_CASE(create_matched_bet_dont_check_better)
{
    auto bet1 = create_bet([](auto& bet) { bet.data.better = "alice"; });
    auto bet2 = create_bet([&](auto& bet) {
        bet.data.better = "alice";
        bet.data.wincase = create_opposite(bet1.data.wincase);
    });

    BOOST_CHECK_NO_THROW(create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec()));

    BOOST_CHECK_EQUAL(1u, db.get_index<matched_bet_index>().indices().size());
}

SCORUM_TEST_CASE(check_matched_stake)
{
    auto bet1 = create_bet([](auto&) {});
    auto bet2 = create_bet([&](auto& bet) { bet.data.wincase = create_opposite(bet1.data.wincase); });

    matched.bet1_matched = ASSET_SCR(1);
    matched.bet2_matched = ASSET_SCR(2);

    create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec());

    BOOST_CHECK_EQUAL(matched.bet1_matched, matched_dba.get().bet1_data.stake);
    BOOST_CHECK_EQUAL(matched.bet2_matched, matched_dba.get().bet2_data.stake);
}

SCORUM_TEST_CASE(check_matched_bet_data)
{
    auto bet1 = create_bet([](auto&) {});
    auto bet2 = create_bet([&](auto& bet) { bet.data.wincase = create_opposite(bet1.data.wincase); });

    create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec());

    BOOST_CHECK(compare_bet_data(matched_dba.get().bet1_data, bet1.data));
    BOOST_CHECK(compare_bet_data(matched_dba.get().bet2_data, bet2.data));
}

SCORUM_TEST_CASE(create_one_matched_bet)
{
    auto bet1 = create_bet([](auto&) {});
    auto bet2 = create_bet([&](auto& bet) { bet.data.wincase = create_opposite(bet1.data.wincase); });

    create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec());

    BOOST_CHECK_EQUAL(1u, db.get_index<matched_bet_index>().indices().size());
}

SCORUM_TEST_CASE(create_two_matched_bet)
{
    auto bet1 = create_bet([](auto&) {});
    auto bet2 = create_bet([&](auto& bet) { bet.data.wincase = create_opposite(bet1.data.wincase); });

    create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec());
    create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec());

    BOOST_CHECK_EQUAL(2u, db.get_index<matched_bet_index>().indices().size());
}

SCORUM_TEST_CASE(check_ids_for_two_matched_bets)
{
    auto bet1 = create_bet([](auto&) {});
    auto bet2 = create_bet([&](auto& bet) { bet.data.wincase = create_opposite(bet1.data.wincase); });

    auto id1 = create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec());
    auto id2 = create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec());

    BOOST_CHECK_EQUAL(0u, id1);
    BOOST_CHECK_EQUAL(1u, id2);
}

SCORUM_TEST_CASE(throw_exception_when_bets_have_different_game)
{
    auto bet1 = create_bet([](auto& bet) { bet.game_uuid = { 1 }; });

    auto bet2 = create_bet([&](auto& bet) {
        bet.game_uuid = { 2 };
        bet.data.wincase = create_opposite(bet1.data.wincase);
    });

    BOOST_CHECK_THROW(create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec()), fc::assert_exception);
}

SCORUM_TEST_CASE(throw_exception_when_bets_have_different_market)
{
    auto bet1 = create_bet([](auto& bet) { bet.data.wincase = total::under({ 3 }); });
    auto bet2 = create_bet([](auto& bet) { bet.data.wincase = total::over({ 2 }); });

    BOOST_CHECK_THROW(create_matched_bet(matched_dba, bet1, bet2, matched, fc::time_point_sec()), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()
}
