#include <boost/test/unit_test.hpp>
#include <scorum/protocol/block_header.hpp>

#include "database_default_integration.hpp"

using namespace scorum::protocol;

BOOST_AUTO_TEST_SUITE(merkle_root_tests)

BOOST_AUTO_TEST_CASE(check_merkle_roots_are_different_odd_tx_in_block)
{
    checksum_type merkle_root_1;
    {
        database_fixture::database_default_integration_fixture fixture;
        fixture.db.applied_block.connect(
            [&](const signed_block& block) { merkle_root_1 = block.transaction_merkle_root; });

        Actor user0 = "user0";
        Actor user1 = "user1";
        Actor user2 = "user2";
        fixture.actor(fixture.initdelegate).create_account(user0);
        fixture.actor(fixture.initdelegate).create_account(user1);
        fixture.actor(fixture.initdelegate).create_account(user2);

        fixture.generate_block();
    }
    checksum_type merkle_root_2;
    {
        database_fixture::database_default_integration_fixture fixture;
        fixture.db.applied_block.connect(
            [&](const signed_block& block) { merkle_root_2 = block.transaction_merkle_root; });

        Actor user0 = "user00";
        Actor user1 = "user1";
        Actor user2 = "user2";
        fixture.actor(fixture.initdelegate).create_account(user0);
        fixture.actor(fixture.initdelegate).create_account(user1);
        fixture.actor(fixture.initdelegate).create_account(user2);

        fixture.generate_block();
    }

    BOOST_REQUIRE(merkle_root_1 != merkle_root_2);
}

BOOST_AUTO_TEST_CASE(check_merkle_roots_are_different_even_tx_in_block)
{
    checksum_type merkle_root_1;
    {
        database_fixture::database_default_integration_fixture fixture;
        fixture.db.applied_block.connect(
            [&](const signed_block& block) { merkle_root_1 = block.transaction_merkle_root; });

        Actor user0 = "user0";
        Actor user1 = "user1";
        fixture.actor(fixture.initdelegate).create_account(user0);
        fixture.actor(fixture.initdelegate).create_account(user1);

        fixture.generate_block();
    }
    checksum_type merkle_root_2;
    {
        database_fixture::database_default_integration_fixture fixture;
        fixture.db.applied_block.connect(
            [&](const signed_block& block) { merkle_root_2 = block.transaction_merkle_root; });

        Actor user0 = "user00";
        Actor user1 = "user1";
        fixture.actor(fixture.initdelegate).create_account(user0);
        fixture.actor(fixture.initdelegate).create_account(user1);

        fixture.generate_block();
    }

    BOOST_REQUIRE(merkle_root_1 != merkle_root_2);
}

BOOST_AUTO_TEST_CASE(check_blockid_depends_on_previous_block)
{
    block_id_type id_1;
    {
        database_fixture::database_default_integration_fixture fixture;
        fixture.db.applied_block.connect(
            [&](const signed_block& block) { id_1 = block.id(); });

        Actor user0 = "user0";
        Actor user1 = "user1";

        fixture.actor(fixture.initdelegate).create_account(user0);
        fixture.generate_block();

        fixture.actor(fixture.initdelegate).create_account(user1);
        fixture.generate_block();
    }
    block_id_type id_2;
    {
        database_fixture::database_default_integration_fixture fixture;
        fixture.db.applied_block.connect(
            [&](const signed_block& block) { id_2 = block.id(); });

        Actor user0 = "user00";
        Actor user1 = "user1";

        fixture.actor(fixture.initdelegate).create_account(user0);
        fixture.generate_block();

        fixture.actor(fixture.initdelegate).create_account(user1);
        fixture.generate_block();
    }

    BOOST_REQUIRE(id_1 != id_2);
}

BOOST_AUTO_TEST_SUITE_END()