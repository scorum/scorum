#include <boost/test/unit_test.hpp>

#include <scorum/app/api_context.hpp>

#include <scorum/app/database_api.hpp>

#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/rewards_math/formulas.hpp>

#include "database_trx_integration.hpp"

#include <rewards/blogging_common.hpp>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;
using namespace scorum::rewards_math;
using fc::string;

namespace account_api_tests {

using namespace database_fixture;

struct account_api_test_fixture : public blogging_common_with_accounts_fixture
{
    account_api_test_fixture()
        : _database_api_ctx(app, "database_api", std::make_shared<api_session_data>())
        , database_api_call(_database_api_ctx)
    {
    }

    api_context _database_api_ctx;
    database_api database_api_call;
};

BOOST_FIXTURE_TEST_SUITE(database_api_account_tests, account_api_test_fixture)

SCORUM_TEST_CASE(check_voting_power)
{
    auto post_permlink = create_next_post_permlink();

    post(alice, post_permlink); // alice post

    const int vote_interval = SCORUM_CASHOUT_WINDOW_SECONDS / 2;

    generate_blocks(db.head_block_time() + vote_interval);

    vote(alice, post_permlink, sam); // sam upvote alice post

    generate_block();

    auto sam_voting_power = account_service.get_account(sam.name).voting_power;

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS - vote_interval);

    BOOST_REQUIRE_EQUAL(account_service.get_account(sam.name).voting_power, sam_voting_power);

    auto api_objs = database_api_call.get_accounts({ sam.name });

    BOOST_REQUIRE_EQUAL(api_objs.size(), 1u);

    extended_account api_obj = api_objs[0];

    auto predicted_voting_power = calculate_restoring_power(
        account_service.get_account(sam.name).voting_power, db.head_block_time(),
        account_service.get_account(sam.name).last_vote_time, SCORUM_VOTE_REGENERATION_SECONDS);

    BOOST_REQUIRE_EQUAL(api_obj.voting_power, predicted_voting_power);
}

BOOST_AUTO_TEST_SUITE_END()
}
