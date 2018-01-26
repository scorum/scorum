#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_vote.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

#include "database_fixture.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

class vote_apply_base_fixture : public clean_database_fixture
{
public:
    vote_apply_base_fixture()
        : account_service(db.account_service())
        , comment_service(db.comment_service())
        , comment_vote_service(db.comment_vote_service())
    {
        ACTORS((alice)(bob)(sam)(dave))

        private_keys["alice"] = alice_private_key;
        private_keys["bob"] = bob_private_key;
        private_keys["sam"] = sam_private_key;
        private_keys["dave"] = dave_private_key;

        generate_block();

        vest("bob", ASSET_SCR(100e+3));
        vest("sam", ASSET_SCR(100e+3));
        vest("dave", ASSET_SCR(100e+3));

        generate_block();

        auto& alice_comment_op = comment_ops["alice"];
        alice_comment_op.author = "alice";
        alice_comment_op.permlink = "foo";
        alice_comment_op.title = "bar";
        alice_comment_op.body = "foo bar";
        alice_comment_op.parent_permlink = "test";

        auto& bob_comment_op = comment_ops["bob"];
        bob_comment_op.author = "bob";
        bob_comment_op.permlink = "foo";
        bob_comment_op.title = "bar";
        bob_comment_op.body = "foo bar";
        bob_comment_op.parent_permlink = "test";

        auto& sam_comment_op = comment_ops["sam"];
        sam_comment_op.author = "sam";
        sam_comment_op.permlink = "foo";
        sam_comment_op.title = "bar";
        sam_comment_op.body = "foo bar";
        sam_comment_op.parent_permlink = "foo";
        sam_comment_op.parent_author = "alice";
    }

    std::string comment(const std::string& actor)
    {
        BOOST_REQUIRE_NO_THROW(validate_database());

        if (comment_ops[actor].parent_author != SCORUM_ROOT_POST_PARENT)
        {
            comment(comment_ops[actor].parent_author);
        }

        signed_transaction tx;

        tx.operations.push_back(comment_ops[actor]);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(private_keys[actor], db.get_chain_id());
        BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));
        BOOST_REQUIRE_NO_THROW(validate_database());

        generate_block();

        return comment_ops[actor].permlink;
    }

    const account_service_i& account_service;
    const comment_service_i& comment_service;
    const comment_vote_service_i& comment_vote_service;

    comment_operation alice_comment_op;
    comment_operation bob_comment_op;
    comment_operation sam_comment_op;

    using keys_type = std::map<std::string, private_key_type>;
    keys_type private_keys;

    using comment_ops_type = std::map<std::string, comment_operation>;
    comment_ops_type comment_ops;
};

BOOST_FIXTURE_TEST_SUITE(vote_apply_1, vote_apply_base_fixture)

SCORUM_TEST_CASE(alice_comment_check)
{
    comment("alice");
}

SCORUM_TEST_CASE(bob_comment_check)
{
    comment("bob");
}

SCORUM_TEST_CASE(sam_comment_check)
{
    comment("sam");
}

SCORUM_TEST_CASE(voting_non_existent_comment_check)
{
    comment("alice");

    vote_operation op;
    op.voter = "bob";
    op.author = "sam"; // Sam has not commented yet
    op.permlink = "foo";
    op.weight = (int16_t)100;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(private_keys["bob"], db.get_chain_id());

    SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
}

SCORUM_TEST_CASE(voting_with_zerro_weight_check)
{
    comment("alice");

    vote_operation op;
    op.voter = "bob";
    op.author = "alice";
    op.permlink = "foo";
    op.weight = (int16_t)0;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(private_keys["bob"], db.get_chain_id());

    SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);
}

SCORUM_TEST_CASE(voting_with_dust_vesting_check)
{
    comment("alice");

    vote_operation op;
    op.voter = "alice"; // Alice has not enough vests to vote
    op.author = "alice";
    op.permlink = "foo";
    op.weight = (int16_t)100;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(private_keys["alice"], db.get_chain_id());

    SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);
}

SCORUM_TEST_CASE(success_vote_voting_power_check)
{
    auto comment_permlink = comment("alice");

    const account_object& bob_vested = account_service.get_account("bob");

    auto old_voting_power = bob_vested.voting_power;
    auto old_last_vote_time = bob_vested.last_vote_time;

    vote_operation op;
    op.voter = "bob"; // bob has enough vests to vote
    op.author = "alice";
    op.permlink = "foo";
    op.weight = (int16_t)100;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(private_keys["bob"], db.get_chain_id());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE_NO_THROW(validate_database());

    auto& alice_comment = db.get_comment("alice", comment_permlink);

    const auto& comment_vote = comment_vote_service.get(alice_comment.id, bob_vested.id);

    //------------------------------------------------------------------------Bob changes

    // check last_vote_time
    BOOST_REQUIRE_EQUAL(bob_vested.last_vote_time.sec_since_epoch(), db.head_block_time().sec_since_epoch());

    // clang-format off
    //
    // check voting_power decrease:
    //
    //                (max_vote - 1)
    // voting_power = -------------- * (old_voting_power - 1)
    //                  max_vote
    //
    // max_vote = vote_power_reserve_rate * scorum_vote_regeneration_days
    //
    // clang-format on

    int64_t max_vote = (db.get_dynamic_global_properties().vote_power_reserve_rate * SCORUM_VOTE_REGENERATION_SECONDS)
        / (60 * 60 * 24);

    BOOST_REQUIRE_EQUAL(bob_vested.voting_power, old_voting_power - ((old_voting_power + max_vote - 1) / max_vote));

    //------------------------------------------------------------Bob comment vote changes

    // clang-format off
    //
    // check comment reward setting (all Bob input values):
    //
    //           ( old_voting_power - voting_power )
    // rshares = ---------------------------------- * vesting_shares
    //                         100
    //
    // clang-format on

    share_type rshares
        = bob_vested.vesting_shares.amount.value * (old_voting_power - bob_vested.voting_power) / SCORUM_100_PERCENT;

    BOOST_REQUIRE_EQUAL(comment_vote.rshares, rshares);

    //--------------------------------------------------------------Alice comment changes

    // clang-format off
    //
    // check abs_rshares -
    //        This is used to track the total abs(weight) of votes.
    //
    //               vesting_shares                             100 * elapsed_seconds             weight                    1
    // abs_rshares = -------------- * (( old_voting_power + -------------------------------- ) * ------- + max_vote - 1) * ---
    //                    100                               scorum_vote_regeneration_seconds       100                   max_vote
    //
    // clang-format on

    int64_t elapsed_seconds = (db.head_block_time() - old_last_vote_time).to_seconds();
    int64_t regenerated_power = (SCORUM_100_PERCENT * elapsed_seconds) / SCORUM_VOTE_REGENERATION_SECONDS;
    int64_t current_power = std::min(int64_t(old_voting_power + regenerated_power), int64_t(SCORUM_100_PERCENT));
    int64_t abs_weight = std::abs(op.weight * SCORUM_1_PERCENT);
    int64_t used_power = (current_power * abs_weight) / SCORUM_100_PERCENT;
    used_power = (used_power + max_vote - 1) / max_vote;
    int64_t abs_rshares
        = ((uint128_t(bob_vested.effective_vesting_shares().amount.value) * used_power) / (SCORUM_100_PERCENT))
              .to_uint64();
    BOOST_REQUIRE_EQUAL(alice_comment.abs_rshares.value, (share_value_type)abs_rshares);

    // Magic for integer arithmetic
    BOOST_REQUIRE_EQUAL(rshares.value, (share_value_type)abs_rshares);

    //
    // check net_rshares
    //
    // net_rshares = rshares, for one vote
    //

    BOOST_REQUIRE_EQUAL(alice_comment.net_rshares.value, rshares.value);

    //
    // check net_rshares
    //
    // net_rshares = rshares, for one positive vote
    //

    BOOST_REQUIRE_EQUAL(alice_comment.vote_rshares.value, rshares.value);

    // check cashout_time
    BOOST_REQUIRE_EQUAL(alice_comment.cashout_time.sec_since_epoch(),
                        (alice_comment.created + SCORUM_CASHOUT_WINDOW_SECONDS).sec_since_epoch());
}

BOOST_AUTO_TEST_SUITE_END()

class vote_apply_for_alice_fixture : public vote_apply_base_fixture
{
public:
    vote_apply_for_alice_fixture()
    {
        vest("alice", ASSET_SCR(100e+3));

        generate_block();
    }
};

BOOST_FIXTURE_TEST_SUITE(vote_apply_2, vote_apply_for_alice_fixture)

SCORUM_TEST_CASE(reduced_power_for_quick_voting_check)
{
    auto comment_permlink = comment("bob");

    const account_object& alice_vested = account_service.get_account("alice");

    auto old_voting_power = alice_vested.voting_power;

    vote_operation op;
    op.voter = "alice";
    op.author = "bob";
    op.permlink = "foo";
    op.weight = (int16_t)(100 / 2);

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(private_keys["alice"], db.get_chain_id());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE_NO_THROW(validate_database());

    auto& bob_comment = db.get_comment("bob", comment_permlink);

    int64_t max_vote = (db.get_dynamic_global_properties().vote_power_reserve_rate * SCORUM_VOTE_REGENERATION_SECONDS)
        / (60 * 60 * 24);

    // with SCORUM_100_PERCENT multiplier
    BOOST_REQUIRE(alice_vested.voting_power
                  == old_voting_power
                      - ((old_voting_power + max_vote - 1) * SCORUM_100_PERCENT / (2 * max_vote * SCORUM_100_PERCENT)));

    // check net_rshares = rshares
    BOOST_REQUIRE(bob_comment.net_rshares.value
                  == alice_vested.vesting_shares.amount.value * (old_voting_power - alice_vested.voting_power)
                      / SCORUM_100_PERCENT);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
