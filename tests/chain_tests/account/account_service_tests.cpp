#include <boost/test/unit_test.hpp>

#include "database_default_integration.hpp"

#include "actor.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/witness.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

#include "database_blog_integration.hpp"

namespace account_service_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

struct account_service_fixture : public database_fixture::database_blog_integration_fixture
{
    Actor alice = "alice";
    Actor bob = "bob";

    account_service_fixture()
        : account_service(db.account_service())
        , dgp_service(db.dynamic_global_property_service())
        , witness_service(db.witness_service())
    {
        try
        {
            open_database();

            actor(initdelegate).create_account(alice);
            actor(initdelegate).give_sp(alice, 1e+3);
            actor(initdelegate).give_scr(alice, 1e+3);

            actor(initdelegate).create_account(bob);
            actor(initdelegate).give_sp(bob, 1e+3);
            actor(initdelegate).give_scr(bob, 1e+3);
        }
        FC_CAPTURE_LOG_AND_RETHROW(())
    }

    void make_witness(const Actor& actor)
    {
        witness_update_operation op;
        op.owner = actor.name;
        op.url = "witness creation";
        op.block_signing_key = actor.private_key.get_public_key();
        push_operation(op, actor.private_key);
    }

    void vote_for_witness(const Actor& voter, const Actor& witness)
    {
        account_witness_vote_operation op;
        op.account = voter.name;
        op.witness = witness.name;
        op.approve = true;
        push_operation(op, voter.private_key);
    }

    account_service_i& account_service;
    dynamic_global_property_service_i& dgp_service;
    witness_service_i& witness_service;
};

BOOST_FIXTURE_TEST_SUITE(account_service_tests, account_service_fixture)

SCORUM_TEST_CASE(create_account_should_increase_total_scorumpower_amount)
{
    Actor piter = "piter";

    scorum::protocol::authority active, owner, posting;

    const auto circulating_capital = dgp_service.get().circulating_capital;
    const auto total_scorumpower = dgp_service.get().total_scorumpower;

    BOOST_CHECK_EQUAL(circulating_capital.amount, 10000000060u);
    BOOST_CHECK_EQUAL(total_scorumpower.amount, 28560u);

    const auto fee = ASSET_SCR(5);

    BOOST_REQUIRE_NO_THROW(
        account_service.create_account(piter.name, alice.name, piter.public_key, "{}", owner, active, posting, fee));

    BOOST_CHECK_EQUAL(dgp_service.get().circulating_capital.amount, circulating_capital.amount);
    BOOST_CHECK_EQUAL(dgp_service.get().total_scorumpower.amount, total_scorumpower.amount + fee.amount);
}

SCORUM_TEST_CASE(create_account_with_delegation_should_increase_total_scorumpower_amount)
{
    Actor piter = "piter";

    scorum::protocol::authority active, owner, posting;

    const auto circulating_capital = dgp_service.get().circulating_capital;
    const auto total_scorumpower = dgp_service.get().total_scorumpower;

    BOOST_CHECK_EQUAL(circulating_capital.amount, 10000000060u);
    BOOST_CHECK_EQUAL(total_scorumpower.amount, 28560u);

    const auto fee = ASSET_SCR(5);
    const auto delegated_sp = ASSET_SP(10);

    BOOST_REQUIRE_NO_THROW(account_service.create_account_with_delegation(
        piter.name, alice.name, piter.public_key, "{}", owner, active, posting, fee, delegated_sp));

    BOOST_CHECK_EQUAL(dgp_service.get().circulating_capital.amount, circulating_capital.amount);
    BOOST_CHECK_EQUAL(dgp_service.get().total_scorumpower.amount, total_scorumpower.amount + fee.amount);
}

SCORUM_TEST_CASE(increase_balance_increase_circulating_capital_check)
{
    const asset to_transfer = ASSET_SCR(500);

    auto old_circulating_capital = dgp_service.get().circulating_capital;

    BOOST_REQUIRE_NO_THROW(account_service.increase_balance(account_service.get_account(alice.name), to_transfer));

    BOOST_CHECK_EQUAL(old_circulating_capital + to_transfer, dgp_service.get().circulating_capital);
}

SCORUM_TEST_CASE(decrease_balance_decrease_circulating_capital_check)
{
    const asset to_transfer = ASSET_SCR(500);

    auto old_circulating_capital = dgp_service.get().circulating_capital;

    BOOST_REQUIRE_NO_THROW(account_service.decrease_balance(account_service.get_account(alice.name), to_transfer));

    BOOST_CHECK_EQUAL(old_circulating_capital - to_transfer, dgp_service.get().circulating_capital);
}

SCORUM_TEST_CASE(increase_scorumpower_increase_dgp_check)
{
    const asset to_transfer = ASSET_SP(500);

    auto old_circulating_capital = dgp_service.get().circulating_capital;
    auto old_total_scorumpower = dgp_service.get().total_scorumpower;

    BOOST_REQUIRE_NO_THROW(account_service.increase_scorumpower(account_service.get_account(alice.name), to_transfer));

    BOOST_CHECK_EQUAL(old_circulating_capital + asset(to_transfer.amount, SCORUM_SYMBOL),
                      dgp_service.get().circulating_capital);
    BOOST_CHECK_EQUAL(old_total_scorumpower + to_transfer, dgp_service.get().total_scorumpower);
}

SCORUM_TEST_CASE(decrease_scorumpower_decrease_dgp_check)
{
    const asset to_transfer = ASSET_SP(500);

    auto old_circulating_capital = dgp_service.get().circulating_capital;
    auto old_total_scorumpower = dgp_service.get().total_scorumpower;

    BOOST_REQUIRE_NO_THROW(account_service.decrease_scorumpower(account_service.get_account(alice.name), to_transfer));

    BOOST_CHECK_EQUAL(old_circulating_capital - asset(to_transfer.amount, SCORUM_SYMBOL),
                      dgp_service.get().circulating_capital);
    BOOST_CHECK_EQUAL(old_total_scorumpower - to_transfer, dgp_service.get().total_scorumpower);
}

SCORUM_TEST_CASE(increase_scorumpower_increase_witness_votes_check)
{
    const asset to_transfer = ASSET_SP(500);

    make_witness(bob);

    vote_for_witness(alice, bob);

    auto old_bob_votes = witness_service.get(bob.name).votes;

    BOOST_REQUIRE_NO_THROW(account_service.increase_scorumpower(account_service.get_account(alice.name), to_transfer));

    BOOST_CHECK_EQUAL(old_bob_votes + to_transfer.amount, witness_service.get(bob.name).votes);
}

SCORUM_TEST_CASE(decrease_scorumpower_decrease_witness_votes_check)
{
    const asset to_transfer = ASSET_SP(500);

    make_witness(bob);

    vote_for_witness(alice, bob);

    auto old_bob_votes = witness_service.get(bob.name).votes;

    BOOST_REQUIRE_NO_THROW(account_service.decrease_scorumpower(account_service.get_account(alice.name), to_transfer));

    BOOST_CHECK_EQUAL(old_bob_votes - to_transfer.amount, witness_service.get(bob.name).votes);
}

SCORUM_TEST_CASE(create_scorumpower_full_check)
{
    const asset to_transfer = ASSET_SCR(500);

    make_witness(bob);

    vote_for_witness(alice, bob);

    auto old_circulating_capital = dgp_service.get().circulating_capital;
    auto old_total_scorumpower = dgp_service.get().total_scorumpower;
    auto old_bob_votes = witness_service.get(bob.name).votes;

    BOOST_REQUIRE_NO_THROW(account_service.create_scorumpower(account_service.get_account(alice.name), to_transfer));

    BOOST_CHECK_EQUAL(old_circulating_capital + to_transfer, dgp_service.get().circulating_capital);
    BOOST_CHECK_EQUAL(old_total_scorumpower + asset(to_transfer.amount, SP_SYMBOL),
                      dgp_service.get().total_scorumpower);
    BOOST_CHECK_EQUAL(old_bob_votes + to_transfer.amount, witness_service.get(bob.name).votes);
}

SCORUM_TEST_CASE(get_by_cashout_time_check)
{
    generate_block();

    BOOST_REQUIRE(account_service.get_by_cashout_time(db.head_block_time()).empty());

    auto cashout = db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD;

    BOOST_REQUIRE(account_service.get_by_cashout_time(cashout).empty());

    auto post = create_post(alice).push();
    post.vote(bob).in_block();

    BOOST_REQUIRE(account_service.get_by_cashout_time(db.head_block_time()).empty());

    cashout = db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD;

    BOOST_REQUIRE_EQUAL(account_service.get_by_cashout_time(cashout).size(), 1u);

    post.vote(alice).in_block();

    cashout = db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD;

    BOOST_REQUIRE_EQUAL(account_service.get_by_cashout_time(cashout).size(), 2u);
}

SCORUM_TEST_CASE(update_voting_power_change_active_sp_holders_cashout_time_check)
{
    generate_block();

    BOOST_REQUIRE_EQUAL(account_service.get_account(alice.name).active_sp_holders_cashout_time.sec_since_epoch(),
                        fc::time_point_sec::maximum().sec_since_epoch());

    BOOST_REQUIRE_EQUAL(account_service.get_account(alice.name).voting_power, SCORUM_100_PERCENT);

    SCORUM_MESSAGE("-- Cashout time is not changing if voting power is not changing");

    account_service.update_voting_power(account_service.get_account(alice.name), SCORUM_100_PERCENT);

    BOOST_REQUIRE_EQUAL(account_service.get_account(alice.name).active_sp_holders_cashout_time.sec_since_epoch(),
                        fc::time_point_sec::maximum().sec_since_epoch());

    SCORUM_MESSAGE("-- Cashout time is changing if voting power is changing to less value");

    account_service.update_voting_power(account_service.get_account(alice.name), SCORUM_100_PERCENT / 2);

    auto cashout = (db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD).sec_since_epoch();

    BOOST_CHECK_EQUAL(account_service.get_account(alice.name).active_sp_holders_cashout_time.sec_since_epoch(),
                      cashout);

    SCORUM_MESSAGE("-- Cashout time is not changing if cashout time has already changed");

    generate_block();

    account_service.update_voting_power(account_service.get_account(alice.name), SCORUM_100_PERCENT);

    BOOST_CHECK_EQUAL(account_service.get_account(alice.name).active_sp_holders_cashout_time.sec_since_epoch(),
                      cashout);
}

BOOST_AUTO_TEST_SUITE_END()
}
