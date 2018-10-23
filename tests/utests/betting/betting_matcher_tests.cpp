#include <boost/test/unit_test.hpp>

#include <scorum/chain/database/database_virtual_operations.hpp>

#include <scorum/chain/betting/betting_matcher.hpp>

#include <scorum/chain/dba/db_accessor_factory.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/chain/dba/db_accessor.hpp>

#include "detail.hpp"
#include "db_mock.hpp"
#include "defines.hpp"
#include "hippomocks.h"

namespace betting_matcher_tests {

using namespace boost::uuids;

using namespace scorum::chain;
using namespace scorum::protocol;

struct no_bets_fixture
{
private:
    db_mock db;
    size_t _counter = 0;

public:
    MockRepository mocks;
    database_virtual_operations_emmiter_i* vops_emiter = mocks.Mock<database_virtual_operations_emmiter_i>();

    dba::db_accessor<pending_bet_object> pending_dba;
    dba::db_accessor<matched_bet_object> matched_dba;
    dba::db_accessor<dynamic_global_property_object> dprops_dba;

    betting_matcher matcher;

    no_bets_fixture()
        : pending_dba(dba::db_accessor<pending_bet_object>(db))
        , matched_dba(dba::db_accessor<matched_bet_object>(db))
        , dprops_dba(dba::db_accessor<dynamic_global_property_object>(db))
        , matcher(*vops_emiter, pending_dba, matched_dba, dprops_dba)
    {
        setup_db();
        setup_mock();
    }

    void setup_db()
    {
        db.add_index<pending_bet_index>();
        db.add_index<matched_bet_index>();
        db.add_index<game_index>();
        db.add_index<dynamic_global_property_index>();
    }

    void setup_mock()
    {
        mocks.OnCall(vops_emiter, database_virtual_operations_emmiter_i::push_virtual_operation).With(_);
        dprops_dba.create([](auto&) {});
    }

    template <typename T> size_t count() const
    {
        return db.get_index<T, by_id>().size();
    }

    template <typename C> const pending_bet_object& create_bet(C&& constructor)
    {
        ++_counter;

        return db.create<pending_bet_object>([&](pending_bet_object& bet) {
            bet.data.uuid = gen_uuid(boost::lexical_cast<std::string>(_counter));

            constructor(bet);
        });
    }
};

BOOST_AUTO_TEST_SUITE(betting_matcher_tests)

BOOST_FIXTURE_TEST_CASE(add_bet_with_no_stake_to_the_close_list, no_bets_fixture)
{
    const auto& bet1 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(10);
        bet.data.bet_odds = odds(3, 2);
        bet.data.wincase = total::over({ 1 });
    });

    const auto& bet2 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(0);
        bet.data.bet_odds = bet1.data.bet_odds.inverted();
        bet.data.wincase = create_opposite(bet1.data.wincase);
    });

    auto bets_to_cancel = matcher.match(bet2, fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(1u, bets_to_cancel.size());
}

BOOST_FIXTURE_TEST_CASE(dont_create_matched_bet_when_stake_is_zero, no_bets_fixture)
{
    const auto& bet1 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(10);
        bet.data.bet_odds = odds(3, 2);
        bet.data.wincase = total::over({ 1 });
    });

    const auto& bet2 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(0);
        bet.data.bet_odds = bet1.data.bet_odds.inverted();
        bet.data.wincase = create_opposite(bet1.data.wincase);
    });

    matcher.match(bet2, fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(0u, count<matched_bet_index>());
}

BOOST_FIXTURE_TEST_CASE(dont_add_bet_with_no_stake_to_the_close_list, no_bets_fixture)
{
    const auto& bet1 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(0);
        bet.data.bet_odds = odds(3, 2);
        bet.data.wincase = total::over({ 1 });
    });

    auto bets_to_cancel = matcher.match(bet1, fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(0u, bets_to_cancel.size());
}

BOOST_FIXTURE_TEST_CASE(dont_match_bets_with_the_same_wincase, no_bets_fixture)
{
    const auto& bet1 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(10);
        bet.data.bet_odds = odds(3, 2);
        bet.data.wincase = total::over({ 1 });
    });

    const auto& bet2 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(10);
        bet.data.bet_odds = bet1.data.bet_odds.inverted();
        bet.data.wincase = bet1.data.wincase;
    });

    matcher.match(bet2, fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(0u, count<matched_bet_index>());
}

BOOST_FIXTURE_TEST_CASE(dont_match_bets_with_the_same_odds, no_bets_fixture)
{
    const auto& bet1 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(10);
        bet.data.bet_odds = odds(3, 2);
        bet.data.wincase = total::over({ 1 });
    });

    const auto& bet2 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(10);
        bet.data.bet_odds = bet1.data.bet_odds;
        bet.data.wincase = create_opposite(bet1.data.wincase);
    });

    matcher.match(bet2, fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(0u, count<matched_bet_index>());
}

BOOST_FIXTURE_TEST_CASE(match_with_two_bets, no_bets_fixture)
{
    const auto one_point_five = odds(3, 2);
    const auto total_over_1 = total::over({ 1 });

    create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(10);

        bet.data.bet_odds = one_point_five;
        bet.data.wincase = total_over_1;
    });

    create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(10);

        bet.data.bet_odds = one_point_five;
        bet.data.wincase = total_over_1;
    });

    const auto& bet3 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(10);

        bet.data.bet_odds = one_point_five.inverted();
        bet.data.wincase = create_opposite(total_over_1);
    });

    matcher.match(bet3, fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(2u, count<matched_bet_index>());
}

struct two_bets_fixture : public no_bets_fixture
{
public:
    bet_data bet1;
    bet_data bet2;

    two_bets_fixture()
    {
        bet1 = create_bet([&](pending_bet_object& bet) {
                   bet.data.stake = ASSET_SCR(10);
                   bet.data.bet_odds = odds(3, 2);
                   bet.data.wincase = total::over({ 1 });
               }).data;

        bet2 = create_bet([&](pending_bet_object& bet) {
                   bet.data.stake = ASSET_SCR(10);
                   bet.data.bet_odds = bet1.bet_odds.inverted();
                   bet.data.wincase = create_opposite(bet1.wincase);
               }).data;
    }
};

BOOST_FIXTURE_TEST_CASE(add_fully_matched_bet_to_cancel_list, two_bets_fixture)
{
    auto bets_to_cancel = matcher.match(pending_dba.get_by<by_uuid>(bet2.uuid), fc::time_point_sec::maximum());

    BOOST_REQUIRE_EQUAL(1u, bets_to_cancel.size());

    BOOST_CHECK(bets_to_cancel.front().get().data.uuid == bet1.uuid);
}

BOOST_FIXTURE_TEST_CASE(expect_virtual_operation_call_on_bets_matching, two_bets_fixture)
{
    auto check_virtual_operation = [&](const scorum::protocol::operation& o) {
        o.visit([](const auto&) { BOOST_FAIL("expected 'bets_matched_operation' call"); },
                [&](const scorum::protocol::bets_matched_operation& op) {
                    BOOST_CHECK(op.bet1_uuid == bet1.uuid);
                    BOOST_CHECK(op.bet2_uuid == bet2.uuid);
                    BOOST_CHECK_EQUAL(op.better1, bet1.better);
                    BOOST_CHECK_EQUAL(op.better2, bet2.better);
                    BOOST_CHECK_EQUAL(op.matched_stake1.amount, 10u);
                    BOOST_CHECK_EQUAL(op.matched_stake2.amount, 5u);
                    BOOST_CHECK_EQUAL(op.matched_bet_id, 0);
                });
    };

    mocks.ExpectCall(vops_emiter, database_virtual_operations_emmiter_i::push_virtual_operation)
        .With(_)
        .Do(check_virtual_operation);

    matcher.match(pending_dba.get_by<by_uuid>(bet2.uuid), fc::time_point_sec::maximum());
}

BOOST_FIXTURE_TEST_CASE(create_one_matched_bet, two_bets_fixture)
{
    matcher.match(pending_dba.get_by<by_uuid>(bet2.uuid), fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(1u, count<matched_bet_index>());
}

BOOST_FIXTURE_TEST_CASE(check_bets_matched_stake, two_bets_fixture)
{
    matcher.match(pending_dba.get_by<by_uuid>(bet2.uuid), fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(10u, matched_dba.get_by<by_id>(0u).bet1_data.stake.amount);
    BOOST_CHECK_EQUAL(5u, matched_dba.get_by<by_id>(0u).bet2_data.stake.amount);
}

BOOST_FIXTURE_TEST_CASE(second_bet_matched_on_five_tockens, two_bets_fixture)
{
    matcher.match(pending_dba.get_by<by_uuid>(bet2.uuid), fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(5u, pending_dba.get_by<by_uuid>(bet2.uuid).data.stake.amount);
}

BOOST_FIXTURE_TEST_CASE(first_bet_matched_on_hole_stake, two_bets_fixture)
{
    matcher.match(pending_dba.get_by<by_uuid>(bet2.uuid), fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(0u, pending_dba.get_by<by_uuid>(bet1.uuid).data.stake.amount);
}

struct three_bets_fixture : public no_bets_fixture
{
    bet_data bet1;
    bet_data bet2;
    bet_data bet3;

    const odds one_point_five = odds(3, 2);
    const total::over total_over_1 = total::over({ 1 });

    three_bets_fixture()
    {
        setup_mock();

        bet1 = create_bet([&](pending_bet_object& bet) {
                   bet.data.stake = ASSET_SCR(10);

                   bet.data.bet_odds = one_point_five;
                   bet.data.wincase = total_over_1;
               }).data;

        bet2 = create_bet([&](pending_bet_object& bet) {
                   bet.data.stake = ASSET_SCR(10);

                   bet.data.bet_odds = one_point_five;
                   bet.data.wincase = total_over_1;
               }).data;

        bet3 = create_bet([&](pending_bet_object& bet) {
                   bet.data.stake = ASSET_SCR(10);

                   bet.data.bet_odds = odds(2, 1);
                   bet.data.wincase = create_opposite(total_over_1);
               }).data;
    }
};

BOOST_FIXTURE_TEST_CASE(dont_match_bets_when_odds_dont_match, three_bets_fixture)
{
    matcher.match(pending_dba.get_by<by_uuid>(bet3.uuid), fc::time_point_sec::maximum());

    BOOST_CHECK_EQUAL(0u, count<matched_bet_index>());
}

BOOST_FIXTURE_TEST_CASE(dont_change_pending_bet_stake_when_matching_dont_occur, three_bets_fixture)
{
    matcher.match(pending_dba.get_by<by_uuid>(bet3.uuid), fc::time_point_sec::maximum());

    BOOST_CHECK(compare_bet_data(bet1, pending_dba.get_by<by_uuid>(bet1.uuid).data));
    BOOST_CHECK(compare_bet_data(bet2, pending_dba.get_by<by_uuid>(bet2.uuid).data));
    BOOST_CHECK(compare_bet_data(bet3, pending_dba.get_by<by_uuid>(bet3.uuid).data));
}

BOOST_FIXTURE_TEST_CASE(stop_matching_when_stake_is_spent, three_bets_fixture)
{
    const auto& bet4 = create_bet([&](pending_bet_object& bet) {
        bet.data.stake = ASSET_SCR(1);

        bet.data.bet_odds = one_point_five.inverted();
        bet.data.wincase = create_opposite(total_over_1);
    });

    matcher.match(bet4, fc::time_point_sec::maximum());

    BOOST_REQUIRE_EQUAL(1u, count<matched_bet_index>());

    BOOST_CHECK(matched_dba.get().bet1_data.uuid == bet1.uuid);
    BOOST_CHECK(matched_dba.get().bet2_data.uuid == bet4.data.uuid);
}

BOOST_AUTO_TEST_SUITE_END()
}
