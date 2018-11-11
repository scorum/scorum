#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/registration_objects.hpp>

#include <scorum/chain/dba/db_accessor.hpp>

#include <scorum/chain/database_exceptions.hpp>

#include "database_trx_integration.hpp"
#include "actor.hpp"

namespace {

using namespace scorum::chain;
using namespace scorum::protocol;

class fixture : public database_fixture::database_trx_integration_fixture
{
public:
    Actor alice = "alice";
    Actor bob = "bob";
    Actor initdelegate = "initdelegate";

    const asset total_supply = asset::from_string("29265074.800016790 SCR");
    const share_type registration_bonus = 100u;

    const asset delegated_amount = asset(25, SP_SYMBOL);

    fixture()
        : reg_pool_dba(db)
        , account_dba(db)
        , dprop_dba(db)
    {
        genesis_state = create_genesis();

        open_database();

        BOOST_CHECK_EQUAL(6u, dprop_dba.get().head_block_number);
        BOOST_CHECK_EQUAL(5u, account_dba.size());

        BOOST_REQUIRE_GT(reg_pool_dba.get().balance.amount, delegated_amount.amount);
        BOOST_REQUIRE_EQUAL(reg_pool_dba.get().delegated.amount, 0u);
    }

    genesis_state_type create_genesis()
    {
        try
        {
            supply_type supply(total_supply.amount.value);

            initdelegate.scorumpower(ASSET_SP(supply.take(5)));
            alice.scorumpower(ASSET_SP(supply.take(5)));

            initdelegate.scorum(ASSET_SCR(supply.take(10)));
            alice.scorum(ASSET_SCR(supply.take(10)));

            const registration_stage single_stage{ 1u, 1u, 100u };

            const auto accounts_supply = alice.scr_amount + initdelegate.scr_amount;

            return Genesis::create()
                .accounts_supply(accounts_supply)
                .accounts(alice, bob, initdelegate)
                .witnesses(initdelegate)
                .rewards_supply(ASSET_SCR(supply.take_rest()))
                .registration_supply(ASSET_SCR(supply.take(1000)))
                .registration_bonus(ASSET_SCR(registration_bonus.value))
                .registration_schedule(single_stage)
                .committee(alice)
                .dev_committee(alice)
                .generate();
        }
        FC_LOG_AND_RETHROW()
    }

    dba::db_accessor<registration_pool_object> reg_pool_dba;
    dba::db_accessor<account_object> account_dba;
    dba::db_accessor<dynamic_global_property_object> dprop_dba;
};

BOOST_AUTO_TEST_SUITE(delegate_sp_from_reg_pool_tests)

SCORUM_FIXTURE_TEST_CASE(account_received_sp_from_reg_pool, fixture)
{
    const auto balance_before_delegation = reg_pool_dba.get().balance.amount;

    auto sam = Actor("sam");
    actor(initdelegate).create_account(sam);
    BOOST_CHECK_EQUAL(ASSET_SP(0), account_dba.get_by<by_name>(sam.name).received_scorumpower);

    actor(alice).delegate_sp_from_reg_pool(sam, delegated_amount);

    BOOST_CHECK_EQUAL(delegated_amount, account_dba.get_by<by_name>(sam.name).received_scorumpower);
    BOOST_CHECK_EQUAL(delegated_amount, reg_pool_dba.get().delegated);
    BOOST_CHECK_EQUAL(balance_before_delegation, reg_pool_dba.get().balance.amount + delegated_amount.amount);
}

SCORUM_FIXTURE_TEST_CASE(account_created_in_genesis_received_sp_from_reg_pool, fixture)
{
    const auto balance_before_delegation = reg_pool_dba.get().balance.amount;

    BOOST_CHECK_EQUAL(ASSET_SP(0), account_dba.get_by<by_name>(bob.name).received_scorumpower);

    actor(alice).delegate_sp_from_reg_pool(bob, delegated_amount);

    BOOST_CHECK_EQUAL(delegated_amount, account_dba.get_by<by_name>(bob.name).received_scorumpower);
    BOOST_CHECK_EQUAL(delegated_amount, reg_pool_dba.get().delegated);
    BOOST_CHECK_EQUAL(balance_before_delegation, reg_pool_dba.get().balance.amount + delegated_amount.amount);
}

SCORUM_FIXTURE_TEST_CASE(account_created_by_committee_received_sp_from_reg_pool, fixture)
{
    const auto balance_before_delegation = reg_pool_dba.get().balance.amount;

    auto sam = Actor("sam");
    actor(alice).create_account_by_committee(sam);
    BOOST_CHECK_EQUAL(ASSET_SP(0), account_dba.get_by<by_name>(sam.name).received_scorumpower);

    actor(alice).delegate_sp_from_reg_pool(sam, delegated_amount);

    BOOST_CHECK_EQUAL(delegated_amount, account_dba.get_by<by_name>(sam.name).received_scorumpower);
    BOOST_CHECK_EQUAL(delegated_amount, reg_pool_dba.get().delegated);
    BOOST_CHECK_EQUAL(balance_before_delegation, reg_pool_dba.get().balance.amount + delegated_amount.amount);
}

SCORUM_FIXTURE_TEST_CASE(cancel_delegation, fixture)
{
    const auto balance_before_delegation = reg_pool_dba.get().balance;

    actor(alice).delegate_sp_from_reg_pool(bob, delegated_amount);

    BOOST_CHECK_EQUAL(delegated_amount, account_dba.get_by<by_name>(bob.name).received_scorumpower);
    BOOST_CHECK_EQUAL(delegated_amount, reg_pool_dba.get().delegated);

    actor(alice).delegate_sp_from_reg_pool(bob, ASSET_SP(0));

    BOOST_CHECK_EQUAL(ASSET_SP(0), account_dba.get_by<by_name>(bob.name).received_scorumpower);
    BOOST_CHECK_EQUAL(ASSET_SP(0), reg_pool_dba.get().delegated);
    BOOST_CHECK_EQUAL(balance_before_delegation, reg_pool_dba.get().balance);
}

SCORUM_FIXTURE_TEST_CASE(account_witout_sp_dont_have_bandwidth, fixture)
{
    auto sam = Actor("sam");
    actor(alice).create_account_by_committee(sam);
    actor(alice).transfer(sam, ASSET_SCR(10));

    SCORUM_CHECK_EXCEPTION(actor(sam).transfer(bob, ASSET_SCR(10)), scorum::chain::plugin_exception,
                           "Account: sam bandwidth limit exceeded. Please wait to transact or power up SCR.");
}

SCORUM_FIXTURE_TEST_CASE(account_received_sp_from_committee_could_apply_ops, fixture)
{
    auto sam = Actor("sam");
    actor(alice).create_account_by_committee(sam);

    actor(alice).delegate_sp_from_reg_pool(sam, ASSET_SP(5'000'000'000));

    actor(alice).transfer(sam, ASSET_SCR(10));

    BOOST_CHECK_EQUAL(ASSET_SCR(0), account_dba.get_by<by_name>(bob.name).balance);

    BOOST_CHECK_NO_THROW(actor(sam).transfer(bob, ASSET_SCR(10)));

    BOOST_CHECK_EQUAL(ASSET_SCR(10), account_dba.get_by<by_name>(bob.name).balance);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace
