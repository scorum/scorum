#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>

#include <scorum/chain/evaluators/withdraw_scorumpower_evaluator.hpp>

#include <scorum/chain/schema/account_objects.hpp>

#include "database_trx_integration.hpp"

#include "actoractions.hpp"

using namespace scorum::chain;

struct withdraw_scorumpower_lock_fixture : public database_fixture::database_trx_integration_fixture
{
    withdraw_scorumpower_lock_fixture()
        : dynamic_global_property_service(db.dynamic_global_property_service())
        , account_service(db.account_service())
        , withdraw_scorumpower_service(db.withdraw_scorumpower_service())
    {
        lock_withdraw_sp_until_timestamp
            = database_integration_fixture::default_genesis_state().generate().initial_timestamp;
        lock_withdraw_sp_until_timestamp += 100 * SCORUM_BLOCK_INTERVAL;

        genesis_state_type genesis = database_integration_fixture::default_genesis_state()
                                         .lock_withdraw_sp_until_timestamp(lock_withdraw_sp_until_timestamp)
                                         .development_sp_supply(to_withdraw_sp)
                                         .dev_committee(alice)
                                         .generate();
        open_database(genesis);
        generate_block();

        actor(initdelegate).give_sp(alice, to_withdraw_sp.amount.value);
    }

    void account_withdraw()
    {
        const auto& account = account_service.get_account(alice.name);

        withdraw_scorumpower_operation op;
        op.account = account.name;
        op.scorumpower = to_withdraw_sp;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice.private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
    }

    fc::time_point_sec lock_withdraw_sp_until_timestamp;
    asset to_withdraw_sp = ASSET_SP(1e+3);
    Actor alice = Actor("alice");

    dynamic_global_property_service_i& dynamic_global_property_service;
    account_service_i& account_service;
    withdraw_scorumpower_service_i& withdraw_scorumpower_service;
};

BOOST_FIXTURE_TEST_SUITE(withdraw_scorumpower_lock_tests, withdraw_scorumpower_lock_fixture)

SCORUM_TEST_CASE(validate_env_check)
{
    BOOST_REQUIRE_GT(lock_withdraw_sp_until_timestamp.sec_since_epoch(),
                     dynamic_global_property_service.head_block_time().sec_since_epoch());
}

SCORUM_TEST_CASE(withdraw_sp_from_account_is_locked_check)
{
    SCORUM_REQUIRE_THROW(account_withdraw(), fc::assert_exception);
}

SCORUM_TEST_CASE(withdraw_sp_from_account_will_be_unlocked_check)
{
    generate_blocks(lock_withdraw_sp_until_timestamp + SCORUM_BLOCK_INTERVAL, true);
    BOOST_REQUIRE_LT(lock_withdraw_sp_until_timestamp.sec_since_epoch(),
                     dynamic_global_property_service.head_block_time().sec_since_epoch());

    BOOST_REQUIRE_NO_THROW(account_withdraw());
}

SCORUM_TEST_CASE(withdraw_sp_from_dev_pool_is_locked_check)
{
    db_plugin->debug_update(
        [&](database&) {
            withdraw_scorumpower_dev_pool_task create_withdraw;
            withdraw_scorumpower_context ctx(db, to_withdraw_sp);
            SCORUM_REQUIRE_THROW(create_withdraw.apply(ctx), fc::assert_exception);
        },
        get_skip_flags());
}

SCORUM_TEST_CASE(withdraw_sp_from_dev_pool_will_be_unlocked_check)
{
    generate_blocks(lock_withdraw_sp_until_timestamp + SCORUM_BLOCK_INTERVAL, true);
    BOOST_REQUIRE_LT(lock_withdraw_sp_until_timestamp.sec_since_epoch(),
                     dynamic_global_property_service.head_block_time().sec_since_epoch());

    db_plugin->debug_update(
        [&](database&) {
            withdraw_scorumpower_dev_pool_task create_withdraw;
            withdraw_scorumpower_context ctx(db, to_withdraw_sp);
            BOOST_REQUIRE_NO_THROW(create_withdraw.apply(ctx));
        },
        get_skip_flags());
}

BOOST_AUTO_TEST_SUITE_END()
