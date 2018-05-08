#include <boost/test/unit_test.hpp>

#include "registration_check_common.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/registration_pool.hpp>

using namespace database_fixture;

struct registration_committee_burning_bonus_fixture : public database_trx_integration_fixture
{
    registration_committee_burning_bonus_fixture()
        : account_service(db.account_service())
        , registration_pool_service(db.registration_pool_service())
        , alice("alice")
        , bob("bob")
    {
        genesis_state_type genesis
            = database_integration_fixture::default_genesis_state()
                  .registration_supply(registration_bonus * 100)
                  .registration_bonus(registration_bonus)
                  .registration_schedule(schedule_input_type{ 1, 2, 100 }, schedule_input_type{ 2, 1, 50 })
                  .committee(initdelegate.name)
                  .generate();

        open_database(genesis);
    }

    void create_account_by_committee(const Actor& new_account)
    {
        account_create_by_committee_operation op;

        op.creator = initdelegate.name;
        op.new_account_name = new_account.name;
        op.owner = authority(1, new_account.public_key, 1);
        op.active = authority(1, new_account.public_key, 1);
        op.posting = authority(1, new_account.public_key, 1);
        op.memo_key = new_account.public_key;
        op.json_metadata = "";

        push_operation_only(op, initdelegate.private_key);

        generate_block();
    }

    void do_activity(const Actor& account)
    {
        comment_operation comment;

        comment.author = account.name;
        comment.permlink = "test";
        comment.parent_permlink = "posts";
        comment.title = "foo";
        comment.body = "bar";

        push_operation_only(comment, account.private_key);

        generate_block();
    }

    const asset registration_bonus = ASSET_SCR(100);
    account_service_i& account_service;
    registration_pool_service_i& registration_pool_service;

    Actor alice;
    Actor bob;
};

BOOST_FIXTURE_TEST_SUITE(registration_committee_burning_bonus, registration_committee_burning_bonus_fixture)

SCORUM_TEST_CASE(burn_sp_for_one_account)
{
    asset old_registration_pool_balance = registration_pool_service.get().balance;

    create_account_by_committee(alice);

    asset alice_bonus = account_service.get_account(alice.name).scorumpower;

    BOOST_REQUIRE_GT(alice_bonus, ASSET_NULL_SP);

    BOOST_REQUIRE_EQUAL(registration_pool_service.get().balance,
                        old_registration_pool_balance - asset(alice_bonus.amount, SCORUM_SYMBOL));

    generate_blocks(db.head_block_time() + SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS.to_seconds());

    BOOST_REQUIRE_EQUAL(account_service.get_account(alice.name).scorumpower, ASSET_NULL_SP);

    BOOST_REQUIRE_EQUAL(registration_pool_service.get().balance, old_registration_pool_balance);

    validate_database();
}

SCORUM_TEST_CASE(burn_sp_for_two_accounts)
{
    create_account_by_committee(alice);

    generate_blocks(db.head_block_time() + SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS.to_seconds() / 2);

    create_account_by_committee(bob);

    BOOST_REQUIRE_GT(account_service.get_account(alice.name).scorumpower, ASSET_NULL_SP);
    BOOST_REQUIRE_GT(account_service.get_account(bob.name).scorumpower, ASSET_NULL_SP);

    generate_blocks(db.head_block_time() + SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS.to_seconds() / 2);

    BOOST_REQUIRE_EQUAL(account_service.get_account(alice.name).scorumpower, ASSET_NULL_SP);
    BOOST_REQUIRE_GT(account_service.get_account(bob.name).scorumpower, ASSET_NULL_SP);

    generate_blocks(db.head_block_time() + SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS.to_seconds() / 2);

    BOOST_REQUIRE_EQUAL(account_service.get_account(bob.name).scorumpower, ASSET_NULL_SP);
}

SCORUM_TEST_CASE(dont_burn_for_active_user)
{
    create_account_by_committee(alice);

    BOOST_REQUIRE_GT(account_service.get_account(alice.name).scorumpower, ASSET_NULL_SP);

    generate_blocks(db.head_block_time() + SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS.to_seconds() / 2);

    do_activity(alice);

    generate_blocks(db.head_block_time() + SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS.to_seconds() / 2);

    BOOST_REQUIRE_GT(account_service.get_account(alice.name).scorumpower, ASSET_NULL_SP);
}

SCORUM_TEST_CASE(dont_burn_for_active_user_burn_for_not_active)
{
    create_account_by_committee(alice);
    create_account_by_committee(bob);

    BOOST_REQUIRE_GT(account_service.get_account(alice.name).scorumpower, ASSET_NULL_SP);
    BOOST_REQUIRE_GT(account_service.get_account(bob.name).scorumpower, ASSET_NULL_SP);

    generate_blocks(db.head_block_time() + SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS.to_seconds() / 2);

    do_activity(alice);

    generate_blocks(db.head_block_time() + SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS.to_seconds() / 2);

    BOOST_REQUIRE_GT(account_service.get_account(alice.name).scorumpower, ASSET_NULL_SP);
    BOOST_REQUIRE_EQUAL(account_service.get_account(bob.name).scorumpower, ASSET_NULL_SP);
}

BOOST_AUTO_TEST_SUITE_END()
