#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/chain/dba/db_accessor.hpp>

#include "database_trx_integration.hpp"
#include "database_betting_integration.hpp"

#include "actor.hpp"

#include "defines.hpp"

namespace {

using namespace scorum::chain;
using namespace scorum::protocol;

class fixture : public database_fixture::database_betting_integration_fixture
{
public:
    Actor alice = "alice";
    Actor bob = "bob";

    const asset total_supply = SCORUM_MAX_BET_STAKE * 4;
    const share_type registration_bonus = 100u;

    fixture()
        : account_dba(db)
        , pending_dba(db)
        , matched_dba(db)
        , dprop_dba(db)
    {
        genesis_state = create_genesis();

        open_database();

        BOOST_CHECK_EQUAL(6u, dprop_dba.get().head_block_number);
        BOOST_CHECK_EQUAL(5u, account_dba.size());
    }

    genesis_state_type create_genesis()
    {
        try
        {
            supply_type pool(total_supply.amount.value);

            const auto rewards_supply = pool.take(100);
            const auto registration_supply = pool.take(100);

            initdelegate.scorumpower(ASSET_SP(pool.take(5)));
            alice.scorumpower(ASSET_SP(pool.take(5)));

            initdelegate.scorum(ASSET_SCR(pool.take(10)));

            alice.scorum(ASSET_SCR(pool.take(pool.amount() / 2)));
            bob.scorum(ASSET_SCR(pool.take_rest()));

            const registration_stage single_stage{ 1u, 1u, 100u };

            const auto accounts_supply = alice.scr_amount + initdelegate.scr_amount + bob.scr_amount;

            return Genesis::create()
                .accounts_supply(accounts_supply)
                .accounts(alice, bob, initdelegate)
                .witnesses(initdelegate)
                .rewards_supply(ASSET_SCR(rewards_supply))
                .registration_supply(ASSET_SCR(registration_supply))
                .registration_bonus(ASSET_SCR(registration_bonus.value))
                .registration_schedule(single_stage)
                .committee(initdelegate)
                .dev_committee(initdelegate)
                .generate();
        }
        FC_LOG_AND_RETHROW()
    }

    dba::db_accessor<account_object> account_dba;
    dba::db_accessor<pending_bet_object> pending_dba;
    dba::db_accessor<matched_bet_object> matched_dba;
    dba::db_accessor<dynamic_global_property_object> dprop_dba;
};

BOOST_AUTO_TEST_SUITE(post_bet_tests)

SCORUM_FIXTURE_TEST_CASE(post_bet_with_max_odds_and_max_stake, fixture)
{
    empower_moderator(alice);
    create_game({ { 1 } }, alice, { total{ 1000 } });

    generate_block();

    create_bet({ { 1 } }, alice, total::over{ 1000 },
               { SCORUM_MIN_ODDS.inverted().numerator, SCORUM_MIN_ODDS.inverted().denominator }, SCORUM_MAX_BET_STAKE);

    generate_block();

    create_bet({ { 2 } }, bob, total::under{ 1000 },
               { SCORUM_MIN_ODDS.base().numerator, SCORUM_MIN_ODDS.base().denominator }, SCORUM_MAX_BET_STAKE);

    generate_block();
}

// TODO this needs to be fixed
SCORUM_FIXTURE_TEST_CASE(dont_accept_oposite_bet_if_during_matching_occured_underflow_execption, fixture)
{
    bool f = false;
    db.post_apply_operation.connect([&](const operation_notification& note) { //
        note.op.visit(
            [&](const bet_cancelled_operation&) { //
                f = true;
            },
            [](const auto&) {});
    });

    empower_moderator(alice);
    create_game({ { 1 } }, alice, { total{ 1000 } });

    generate_block();

    BOOST_CHECK_EQUAL(0u, pending_dba.size());
    BOOST_CHECK_EQUAL(0u, matched_dba.size());

    create_bet({ { 1 } }, alice, total::over{ 1000 },
               { SCORUM_MIN_ODDS.inverted().numerator, SCORUM_MIN_ODDS.inverted().denominator },
               SCORUM_MAX_BET_STAKE + 10);

    generate_block();

    BOOST_CHECK_EQUAL(1u, pending_dba.size());
    BOOST_CHECK_EQUAL(0u, matched_dba.size());

    BOOST_REQUIRE_THROW(create_bet({ { 2 } }, bob, total::under{ 1000 },
                                   { SCORUM_MIN_ODDS.base().numerator, SCORUM_MIN_ODDS.base().denominator },
                                   ASSET_SCR(1'000'000'000)),
                        fc::underflow_exception);

    BOOST_CHECK_EQUAL(1u, pending_dba.size());
    BOOST_CHECK_EQUAL(0u, matched_dba.size());

    const auto& bet = pending_dba.get_by<by_uuid>(boost::uuids::uuid({ { 1 } }));

    BOOST_CHECK_EQUAL(SCORUM_MAX_BET_STAKE + 10, bet.data.stake);

    BOOST_CHECK_EQUAL(f, false);
}

SCORUM_FIXTURE_TEST_CASE(dont_accept_oposite_bet_if_during_matching_occured_underflow_execption2, fixture)
{
    auto bet_cancelled_counter = 0u;
    db.post_apply_operation.connect([&](const operation_notification& note) { //
        note.op.visit(
            [&](const bet_cancelled_operation&) { //
                ++bet_cancelled_counter;
            },
            [](const auto&) {});
    });

    empower_moderator(alice);
    create_game({ { 1 } }, alice, { total{ 1000 } });

    generate_block();

    BOOST_CHECK_EQUAL(0u, pending_dba.size());
    BOOST_CHECK_EQUAL(0u, matched_dba.size());

    create_bet({ { 1 } }, bob, total::under{ 1000 },
               { SCORUM_MIN_ODDS.base().numerator, SCORUM_MIN_ODDS.base().denominator }, ASSET_SCR(1'000'000'000));

    generate_block();

    BOOST_CHECK_EQUAL(1u, pending_dba.size());
    BOOST_CHECK_EQUAL(0u, matched_dba.size());

    BOOST_REQUIRE_THROW(create_bet({ { 2 } }, alice, total::over{ 1000 },
                                   { SCORUM_MIN_ODDS.inverted().numerator, SCORUM_MIN_ODDS.inverted().denominator },
                                   SCORUM_MAX_BET_STAKE + 10),
                        fc::underflow_exception);

    BOOST_CHECK_EQUAL(1u, pending_dba.size());
    BOOST_CHECK_EQUAL(0u, matched_dba.size());

    const auto& bet = pending_dba.get_by<by_uuid>(boost::uuids::uuid({ { 1 } }));

    BOOST_CHECK_EQUAL(ASSET_SCR(1'000'000'000), bet.data.stake);
    BOOST_CHECK_EQUAL(0u, bet_cancelled_counter);
}

SCORUM_FIXTURE_TEST_CASE(bet_cancelled_operation_emitted_on_full_bet_matching, fixture)
{
    auto bet_cancelled_counter = 0u;

    db.post_apply_operation.connect([&](const operation_notification& note) { //
        note.op.visit(
            [&](const bet_cancelled_operation& op) { //
                ++bet_cancelled_counter;

                BOOST_CHECK(op.bet_uuid == boost::uuids::uuid({ { 1 } }));
            },
            [](const auto&) {});
    });

    empower_moderator(alice);
    create_game({ { 1 } }, alice, { total{ 1000 } });

    generate_block();

    BOOST_CHECK_EQUAL(0u, pending_dba.size());
    BOOST_CHECK_EQUAL(0u, matched_dba.size());

    create_bet({ { 1 } }, bob, total::under{ 1000 }, { 3, 2 }, ASSET_SCR(1'000'000'000));

    generate_block();

    BOOST_CHECK_EQUAL(1u, pending_dba.size());
    BOOST_CHECK_EQUAL(0u, matched_dba.size());

    create_bet({ { 2 } }, alice, total::over{ 1000 }, { 3, 1 }, ASSET_SCR(1'000'000'000));

    BOOST_CHECK_EQUAL(1u, pending_dba.size());
    BOOST_CHECK_EQUAL(1u, matched_dba.size());
    BOOST_CHECK_EQUAL(1u, bet_cancelled_counter);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace
