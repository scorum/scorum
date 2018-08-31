#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include "database_default_integration.hpp"
#include "actor.hpp"

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/protocol/betting/market_kind.hpp>

namespace {

using namespace scorum::protocol;
using namespace scorum::protocol::betting;
using namespace scorum::chain;

struct game_operations_fixture : public database_fixture::database_default_integration_fixture
{
    game_operations_fixture()
        : dgp_service(db.dynamic_global_property_service())
        , betting_property_service(db.betting_property_service())
        , account_service(db.account_service())
        , game_service(db.game_service())
    {
        alice.scorum(ASSET_SCR(1e+9));
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_sp(alice, 1e+9);
        actor(initdelegate).give_scr(alice, alice.scr_amount.amount.value);

        bob.scorum(ASSET_SCR(1e+9));
        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_sp(bob, 1e+9);
        actor(initdelegate).give_scr(bob, bob.scr_amount.amount.value);

        actor(initdelegate).create_account(moderator);
        actor(initdelegate).give_sp(moderator, 1e+9);
        actor(initdelegate).give_scr(moderator, 1e+9);

        empower_moderator(moderator);

        BOOST_REQUIRE(betting_property_service.is_exists());
        BOOST_REQUIRE_EQUAL(betting_property_service.get().moderator, moderator.name);
    }

    template <typename... TMarkets>
    create_game_operation create_game(const Actor& moderator, const TMarkets&... markets)
    {
        try
        {
            create_game_operation op;
            op.moderator = moderator.name;
            op.start = dgp_service.head_block_time() + fc::seconds(3);
            op.name = "test";
            op.game = soccer_game{};
            op.markets = { markets... };

            push_operation_only(op, moderator.private_key);

            return op;
        }
        FC_CAPTURE_LOG_AND_RETHROW(())
    }

    void empower_moderator(const Actor& moderator)
    {
        try
        {
            development_committee_empower_betting_moderator_operation operation;
            operation.account = moderator.name;

            {
                proposal_create_operation op;
                op.creator = initdelegate.name;
                op.operation = operation;
                op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;

                push_operation_only(op, initdelegate.private_key);
            }

            {
                proposal_vote_operation op;
                op.voting_account = initdelegate.name;
                op.proposal_id = get_last_proposal_id()._id;

                push_operation_only(op, initdelegate.private_key);
            }

            generate_block();
        }
        FC_CAPTURE_LOG_AND_RETHROW(())
    }

    proposal_id_type get_last_proposal_id()
    {
        auto& proposal_service = db.obtain_service<dbs_proposal>();
        std::vector<proposal_object::cref_type> proposals = proposal_service.get_proposals();

        BOOST_REQUIRE_GT(proposals.size(), static_cast<size_t>(0));

        return proposals[proposals.size() - 1].get().id;
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor moderator = "smit";

    dynamic_global_property_service_i& dgp_service;
    betting_property_service_i& betting_property_service;
    account_service_i& account_service;
    game_service_i& game_service;
};

BOOST_FIXTURE_TEST_SUITE(post_game_results_operation_tests, game_operations_fixture)

SCORUM_TEST_CASE(post_game_results_positive_test)
{
    // clang-format off
    create_game(moderator,
            result_home_market{},
            total_market{ 2000 },
            total_market{ 500 },
            correct_score_parametrized_market{ 0, 0 });
    // clang-format on

    auto creation_time = db.head_block_time();

    generate_block();

    const auto& game = db.get(game_object::id_type(0));

    BOOST_REQUIRE_EQUAL((int)game.status, (int)game_status::started);
    BOOST_REQUIRE_EQUAL(game.results.size(), 0u);
    BOOST_REQUIRE_EQUAL(game.last_update.to_iso_string(), creation_time.to_iso_string());

    post_game_results_operation op;
    op.moderator = moderator.name;
    op.game_id = 0;
    op.wincases = { result_draw_away{}, total_over{ 500 }, correct_score_no{ 0, 0 } };

    auto update_time = db.head_block_time();

    push_operation(op, moderator.private_key);

    BOOST_REQUIRE_EQUAL((int)game.status, (int)game_status::finished);
    BOOST_REQUIRE_EQUAL(game.results.size(), 3u);
    BOOST_REQUIRE_EQUAL(game.bets_resolve_time.to_iso_string(),
                        (update_time + SCORUM_BETTING_RESOLVE_DELAY_SEC).to_iso_string());
}

SCORUM_TEST_CASE(post_game_results_twice_positive_test)
{
    // clang-format off
    create_game(moderator,
                result_home_market{},
                total_market{ 2000 },
                total_market{ 500 },
                correct_score_parametrized_market{ 0, 0 });
    // clang-format on

    generate_block();

    const auto& game = db.get(game_object::id_type(0));

    post_game_results_operation op;
    op.moderator = moderator.name;
    op.game_id = 0;
    op.wincases = { result_draw_away{}, total_over{ 500 }, correct_score_no{ 0, 0 } };

    auto fst_update_time = db.head_block_time();

    push_operation(op, moderator.private_key);

    BOOST_REQUIRE_EQUAL((int)game.status, (int)game_status::finished);
    BOOST_REQUIRE_EQUAL(game.results.size(), 3u);
    BOOST_REQUIRE_EQUAL(game.last_update.to_iso_string(), fst_update_time.to_iso_string());
    BOOST_REQUIRE_EQUAL(game.bets_resolve_time.to_iso_string(),
                        (fst_update_time + SCORUM_BETTING_RESOLVE_DELAY_SEC).to_iso_string());

    auto snd_update_time = db.head_block_time();

    op.wincases = { result_draw_away{}, total_under{ 500 }, total_under{ 2000 }, correct_score_no{ 0, 0 } };
    push_operation(op, moderator.private_key);

    BOOST_REQUIRE_EQUAL((int)game.status, (int)game_status::finished);
    BOOST_REQUIRE_EQUAL(game.results.size(), 4u);
    BOOST_REQUIRE_EQUAL(game.last_update.to_iso_string(), snd_update_time.to_iso_string());
    BOOST_REQUIRE_EQUAL(game.bets_resolve_time.to_iso_string(),
                        (fst_update_time + SCORUM_BETTING_RESOLVE_DELAY_SEC).to_iso_string());
}

BOOST_AUTO_TEST_SUITE_END()
}
