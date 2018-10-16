#include <boost/test/unit_test.hpp>

#include "registration_check_common.hpp"

using namespace database_fixture;

class registration_committee_create_account_check_fixture : public registration_check_fixture
{
public:
    registration_committee_create_account_check_fixture()
        : creator("alice")
        , new_account("andrew")
        , amount_of_accounts(db.get_index<account_index, by_id>().size())
    {
        this->set_hardfork(SCORUM_HARDFORK_0_2);

        genesis_state = create_registration_genesis();

        create_registration_objects(genesis_state);

        account_committee_op.creator = creator.name;
        account_committee_op.new_account_name = new_account.name;
        account_committee_op.owner = authority(1, new_account.public_key, 1);
        account_committee_op.active = authority(1, new_account.public_key, 1);
        account_committee_op.posting = authority(1, new_account.public_key, 1);
        account_committee_op.memo_key = new_account.public_key;
        account_committee_op.json_metadata = "";

        // Only "initdelegate" has money. He gift some to creator
        transfer_to_scorumpower("initdelegate", creator.name, 100);

        generate_block();

        BOOST_REQUIRE_NO_THROW(validate_database());

        BOOST_REQUIRE_EQUAL(5u, amount_of_accounts);
        BOOST_REQUIRE_EQUAL(10925, registration_supply().amount);
    }

    template <typename T> void push_operation(T op)
    {
        signed_transaction tx;

        BOOST_REQUIRE_NO_THROW(op.validate());

        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);

        BOOST_REQUIRE_NO_THROW(tx.sign(creator.private_key, db.get_chain_id()));
        BOOST_REQUIRE_NO_THROW(tx.validate());

        BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));
    }

    const Actor creator;
    const Actor new_account;

    account_create_by_committee_operation account_committee_op;

    const size_t amount_of_accounts;
};

BOOST_FIXTURE_TEST_SUITE(registration_committee_create_account_operation_check,
                         registration_committee_create_account_check_fixture)

SCORUM_TEST_CASE(create_account_by_committee_operation_check)
{
    BOOST_REQUIRE_NO_THROW(account_committee_op.validate());
}

SCORUM_TEST_CASE(registration_committee_operation_check_invalid_account_name)
{
    account_committee_op.new_account_name = "";

    BOOST_REQUIRE_THROW(account_committee_op.validate(), fc::assert_exception);

    account_committee_op.new_account_name = "wrong;\n'j'";

    BOOST_REQUIRE_THROW(account_committee_op.validate(), fc::assert_exception);

    account_committee_op.creator = "";

    BOOST_REQUIRE_THROW(account_committee_op.validate(), fc::assert_exception);

    account_committee_op.creator = "wrong;\n'j'";

    BOOST_REQUIRE_THROW(account_committee_op.validate(), fc::assert_exception);
}

SCORUM_TEST_CASE(registration_committee_operation_invalid_json_metadata)
{
    account_committee_op.json_metadata = "\xff\x20\xbf";

    BOOST_REQUIRE_THROW(account_committee_op.validate(), fc::assert_exception);
}

SCORUM_TEST_CASE(create_account_by_committee_check)
{
    account_create_by_committee_operation op;

    op.creator = creator.name;
    op.new_account_name = new_account.name;
    op.owner = authority(1, new_account.public_key, 1);
    op.active = authority(1, new_account.public_key, 1);
    op.posting = authority(1, new_account.public_key, 1);
    op.memo_key = new_account.public_key;
    op.json_metadata = "";

    signed_transaction tx;

    BOOST_REQUIRE_NO_THROW(op.validate());

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(op);

    BOOST_REQUIRE_NO_THROW(tx.sign(creator.private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    const account_object& account = account_service.get_account(new_account.name);

    BOOST_CHECK_GT(account.scorumpower, asset(0, SP_SYMBOL));

    BOOST_REQUIRE_NO_THROW(validate_database());
}

SCORUM_TEST_CASE(create_account_by_committee_no_member_check)
{
    ACTORS((kate))

    generate_block();

    account_create_by_committee_operation op;

    op.creator = "kate";
    op.new_account_name = new_account.name;
    op.owner = authority(1, new_account.public_key, 1);
    op.active = authority(1, new_account.public_key, 1);
    op.posting = authority(1, new_account.public_key, 1);
    op.memo_key = new_account.public_key;
    op.json_metadata = "";

    signed_transaction tx;

    BOOST_REQUIRE_NO_THROW(op.validate());

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(op);

    BOOST_REQUIRE_NO_THROW(tx.sign(kate_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);
}

SCORUM_TEST_CASE(create_account_after_hf_0_3)
{
    set_hardfork(SCORUM_HARDFORK_0_3);

    account_create_by_committee_operation op;

    op.creator = creator.name;
    op.new_account_name = new_account.name;
    op.owner = authority(1, new_account.public_key, 1);
    op.active = authority(1, new_account.public_key, 1);
    op.posting = authority(1, new_account.public_key, 1);
    op.memo_key = new_account.public_key;
    op.json_metadata = "";

    const auto pool_balance_before = db.get<registration_pool_object>().balance;

    BOOST_CHECK_EQUAL(pool_balance_before, registration_supply());

    BOOST_REQUIRE_NO_THROW(push_operation(op));

    const account_object& account = account_service.get_account(new_account.name);

    BOOST_CHECK_EQUAL(account.scorumpower, asset(0, SP_SYMBOL));
    BOOST_CHECK_EQUAL(account.balance, asset(0, SCORUM_SYMBOL));

    const auto pool_balance_after = db.get<registration_pool_object>().balance;

    BOOST_CHECK_EQUAL(pool_balance_before, pool_balance_after);
}

BOOST_AUTO_TEST_SUITE_END()
