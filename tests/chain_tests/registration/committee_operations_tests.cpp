#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include "registration_check_common.hpp"

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace registration_fixtures;

class registration_committee_create_account_check_fixture : public registration_check_fixture
{
public:
    registration_committee_create_account_check_fixture()
    {
        genesis_state = create_registration_genesis(committee_private_keys);
        create_registration_objects(genesis_state);

        new_account_private_key = generate_private_key(new_account_name);
        public_key_type new_account_public_key = new_account_private_key.get_public_key();

        account_committee_op.creator = creator_name;
        account_committee_op.new_account_name = new_account_name;
        account_committee_op.owner = authority(1, new_account_public_key, 1);
        account_committee_op.active = authority(1, new_account_public_key, 1);
        account_committee_op.posting = authority(1, new_account_public_key, 1);
        account_committee_op.memo_key = new_account_public_key;
        account_committee_op.json_metadata = "";

        // Only "initdelegate" has money. He gift some to creator
        transfer_to_scorumpower("initdelegate", creator_name, 100);
    }

    committee_private_keys_type committee_private_keys;
    const account_name_type creator_name = "alice";
    const account_name_type new_account_name = "andrew";

    private_key_type new_account_private_key;
    account_create_by_committee_operation account_committee_op;
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
    generate_block();

    BOOST_REQUIRE_NO_THROW(validate_database());

    account_create_by_committee_operation op;

    private_key_type creator_priv_key = committee_private_keys[creator_name];
    public_key_type new_account_public_key = new_account_private_key.get_public_key();

    op.creator = creator_name;
    op.new_account_name = new_account_name;
    op.owner = authority(1, new_account_public_key, 1);
    op.active = authority(1, new_account_public_key, 1);
    op.posting = authority(1, new_account_public_key, 1);
    op.memo_key = new_account_public_key;
    op.json_metadata = "";

    signed_transaction tx;

    BOOST_REQUIRE_NO_THROW(op.validate());

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(op);

    BOOST_REQUIRE_NO_THROW(tx.sign(creator_priv_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    const account_object& account = account_service.get_account(new_account_name);

    BOOST_CHECK_GT(account.scorumpower, asset(0, SP_SYMBOL));

    BOOST_REQUIRE_NO_THROW(validate_database());
}

SCORUM_TEST_CASE(create_account_by_committee_no_member_check)
{
    ACTORS((kate))

    generate_block();

    account_create_by_committee_operation op;

    op.creator = "kate";
    op.new_account_name = new_account_name;
    op.owner = authority(1, new_account_private_key.get_public_key(), 1);
    op.active = authority(1, new_account_private_key.get_public_key(), 1);
    op.posting = authority(1, new_account_private_key.get_public_key(), 1);
    op.memo_key = new_account_private_key.get_public_key();
    op.json_metadata = "";

    signed_transaction tx;

    BOOST_REQUIRE_NO_THROW(op.validate());

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(op);

    BOOST_REQUIRE_NO_THROW(tx.sign(kate_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
