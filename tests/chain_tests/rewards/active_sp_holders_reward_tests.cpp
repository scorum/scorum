#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include "database_default_integration.hpp"

namespace database_fixture {

class sp_holders_reward_fixture : public database_default_integration_fixture
{
public:
    sp_holders_reward_fixture()
        : budget_service(db.obtain_service<dbs_budget>())
        , account_service(db.obtain_service<dbs_account>())
        , alice("alice")
        , bob("bob")
    {
        actor(initdelegate).create_account(alice);
        // actor(initdelegate).give_scr(alice, feed_amount);
        actor(initdelegate).give_sp(alice, feed_amount);

        actor(initdelegate).create_account(bob);
        // actor(initdelegate).give_scr(bob, feed_amount);
        actor(initdelegate).give_sp(bob, feed_amount);
    }

    dbs_budget& budget_service;
    dbs_account& account_service;

    Actor alice;
    Actor bob;

    const int feed_amount = 10000;
};

} // database_fixture

using namespace scorum::chain;
using namespace scorum::protocol;

BOOST_FIXTURE_TEST_SUITE(active_sp_holders_reward_tests, database_fixture::sp_holders_reward_fixture)

SCORUM_TEST_CASE(per_block_payment)
{
    generate_block();

    try
    {
        comment_operation comment;
        comment.author = "alice";
        comment.permlink = "test";
        comment.parent_permlink = "test";
        comment.title = "test";
        comment.body = "foobar";

        push_operation(comment);

        asset bob_sp_before = db.obtain_service<dbs_account>().get_account("bob").scorumpower;

        vote_operation vote;
        vote.author = "alice";
        vote.permlink = "test";
        vote.voter = "bob";
        vote.weight = (int16_t)100;

        push_operation(vote);

        auto initial_per_block_reward = budget_service.get_fund_budget().per_block;

        auto active_sp_holders_reward
            = initial_per_block_reward * SCORUM_ACTIVE_SP_HOLDERS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;

        BOOST_REQUIRE_EQUAL(account_service.get_account(bob.name).scorumpower,
                            bob_sp_before + active_sp_holders_reward);

        // BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").balance, ASSET_SCR(0));
        // BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("sam").balance, ASSET_SCR(0));

        // asset bob_sp_before = db.obtain_service<dbs_account>().get_account("bob").scorumpower;
        // asset sam_sp_before = db.obtain_service<dbs_account>().get_account("sam").scorumpower;

        // BOOST_REQUIRE_EQUAL(op.from, alice);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
