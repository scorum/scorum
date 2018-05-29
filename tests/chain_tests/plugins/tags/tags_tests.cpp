#include <boost/test/unit_test.hpp>
#include <scorum/chain/services/budget.hpp>

#include <scorum/tags/tags_api_impl.hpp>

#include "tags_common.hpp"

using namespace scorum::tags;

BOOST_FIXTURE_TEST_SUITE(tags_tests, database_fixture::tags_fixture)

SCORUM_TEST_CASE(get_discussions_by_created)
{
    api::discussion_query query;
    query.limit = 1;

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 0u);

    create_post(initdelegate).in_block();

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 1u);
}

SCORUM_TEST_CASE(get_discussions_by_created_return_post_after_cashout_time)
{
    api::discussion_query query;
    query.limit = 1;

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 0u);

    create_post(initdelegate).in_block(SCORUM_CASHOUT_WINDOW_SECONDS);

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 1u);
}

SCORUM_TEST_CASE(test_comments_depth_counter)
{
    auto get_comments_with_depth = [](scorum::chain::database& db) {
        std::map<std::string, uint16_t> result;

        const auto& index = db.get_index<comment_index, by_parent>();

        for (auto itr = index.begin(); itr != index.end(); ++itr)
        {
            result.insert(std::make_pair(fc::to_string(itr->permlink), itr->depth));
        }

        return result;
    };

    auto root = create_post(initdelegate).in_block_with_delay();
    auto root_child = root.create_comment(initdelegate).in_block_with_delay();
    auto root_child_child = root_child.create_comment(initdelegate).in_block_with_delay();

    auto check_list = get_comments_with_depth(this->db);

    BOOST_REQUIRE_EQUAL(0u, check_list[root.permlink()]);
    BOOST_REQUIRE_EQUAL(1u, check_list[root_child.permlink()]);
    BOOST_REQUIRE_EQUAL(2u, check_list[root_child_child.permlink()]);
}

SCORUM_TEST_CASE(get_discussions_by_created_return_post_without_it_comments_if_its_id_0)
{
    auto root = create_post(initdelegate).in_block_with_delay();
    root.create_comment(initdelegate).in_block_with_delay();

    api::discussion_query query;
    query.limit = 100;

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 1u);
}

SCORUM_TEST_CASE(get_discussions_by_created_return_two_posts)
{
    auto post1 = create_post(initdelegate).in_block_with_delay();
    generate_block();
    auto post2 = create_post(initdelegate).in_block_with_delay();
    auto comment_level_1 = post2.create_comment(initdelegate).in_block_with_delay();
    comment_level_1.create_comment(initdelegate).in_block_with_delay();

    api::discussion_query query;
    query.limit = 100;

    const auto discussions = _api.get_discussions_by_created(query);

    BOOST_CHECK_EQUAL(discussions.size(), 2u);

    BOOST_CHECK_EQUAL(discussions[0].permlink, post2.permlink());
    BOOST_CHECK_EQUAL(discussions[1].permlink, post1.permlink());
}

SCORUM_TEST_CASE(post_without_tags_creates_one_empty_tag)
{
    auto& index = db.get_index<scorum::tags::tag_index, scorum::tags::by_comment>();

    BOOST_REQUIRE_EQUAL(0u, index.size());

    create_post(initdelegate).in_block_with_delay();

    BOOST_REQUIRE_EQUAL(1u, index.size());

    auto itr = index.begin();

    BOOST_CHECK_EQUAL(itr->tag, "");
}

#ifndef IS_LOW_MEM
SCORUM_TEST_CASE(do_not_create_tag_object_for_comment)
{
    auto& index = db.get_index<scorum::tags::tag_index, scorum::tags::by_comment>();

    BOOST_REQUIRE_EQUAL(0u, index.size());

    auto post = create_post(initdelegate).in_block_with_delay();
    post.create_comment(initdelegate).set_json(R"({"tags" : ["football"]})").in_block_with_delay();

    BOOST_REQUIRE_EQUAL(1u, index.size());

    auto itr = index.begin();

    BOOST_CHECK_EQUAL(itr->tag, "");
    BOOST_CHECK(itr->comment == 0u);
}

SCORUM_TEST_CASE(create_tags_from_json_metadata)
{
    create_post(initdelegate).set_json(R"({"tags" : ["football", "tenis"]})").in_block_with_delay();

    auto& index = db.get_index<scorum::tags::tag_index, scorum::tags::by_comment>();
    BOOST_REQUIRE_EQUAL(3u, index.size());

    auto itr = index.begin();

    BOOST_CHECK_EQUAL(itr->tag, "");

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "football");

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "tenis");
}

SCORUM_TEST_CASE(create_two_posts_with_same_tags)
{
    create_post(initdelegate).set_json(R"({"tags" : ["football"]})").in_block_with_delay();
    generate_block();
    create_post(initdelegate).set_json(R"({"tags" : ["football"]})").in_block_with_delay();

    auto& index = db.get_index<scorum::tags::tag_index, scorum::tags::by_comment>();
    BOOST_REQUIRE_EQUAL(4u, index.size());

    auto itr = index.begin();

    BOOST_CHECK_EQUAL(itr->tag, "");
    BOOST_CHECK(itr->comment == 0u);

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "football");
    BOOST_CHECK(itr->comment == 0u);

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "");
    BOOST_CHECK(itr->comment == 1u);

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "football");
    BOOST_CHECK(itr->comment == 1u);
}

SCORUM_TEST_CASE(do_not_remove_tag_after_cachout_time)
{
    auto& index = db.get_index<scorum::tags::tag_index, scorum::tags::by_comment>();

    BOOST_REQUIRE_EQUAL(0u, index.size());

    create_post(initdelegate).set_json(R"({"tags" : ["football"]})").in_block(SCORUM_CASHOUT_WINDOW_SECONDS);

    // first one is a 'football' tag; the second one is an empty one (whicn is used to track 'get_discussions_by'
    // request with empty tags
    BOOST_REQUIRE_EQUAL(2u, index.size());
}
#endif

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(calc_pending_payout)

template <asset_symbol_type SymbolType> struct stub_fund
{
    asset activity_reward_balance = asset(0, SymbolType);
    fc::uint128_t recent_claims = 0;
    curve_id author_reward_curve;

    asset_symbol_type symbol() const
    {
        return activity_reward_balance.symbol();
    }
};

using stub_fund_scr = stub_fund<SCORUM_SYMBOL>;
using stub_fund_sp = stub_fund<SP_SYMBOL>;

SCORUM_TEST_CASE(do_not_throw_and_return_zero_when_discussion_and_funds_are_empty)
{
    stub_fund_scr fund_scr;
    stub_fund_sp fund_sp;

    api::discussion d;

    BOOST_REQUIRE_NO_THROW(scorum::tags::calc_pending_payout<stub_fund_scr>(d, fund_scr));
    BOOST_REQUIRE_NO_THROW(scorum::tags::calc_pending_payout<stub_fund_sp>(d, fund_sp));

    BOOST_CHECK_EQUAL(scorum::tags::calc_pending_payout(d, fund_scr), ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(scorum::tags::calc_pending_payout(d, fund_sp), ASSET_NULL_SP);
}

BOOST_AUTO_TEST_SUITE_END()

struct get_content_fixture : public database_fixture::tags_fixture
{
    get_content_fixture()
    {
        actor(initdelegate).give_sp(alice, 1e9);
    }
};

BOOST_FIXTURE_TEST_SUITE(get_content, get_content_fixture)

SCORUM_TEST_CASE(check_total_payout)
{
    actor(initdelegate)
        .create_budget("permlink", asset::from_string("5.000000000 SCR"), db.head_block_time() + fc::days(30));

    auto post = create_post(initdelegate).in_block();
    generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL);

    auto d = _api.get_content(post.author(), post.permlink());

    BOOST_REQUIRE_EQUAL(d.total_payout_scr_value, ASSET_NULL_SCR);
    BOOST_REQUIRE_EQUAL(d.total_payout_sp_value, ASSET_NULL_SP);

    post.vote(alice).in_block();

    generate_blocks(post.cashout_time());

    d = _api.get_content(post.author(), post.permlink());

    BOOST_CHECK_EQUAL(d.total_payout_scr_value, asset::from_string("0.000000007 SCR"));
    BOOST_CHECK_EQUAL(d.total_payout_sp_value, asset::from_string("0.000001040 SP"));
}

SCORUM_TEST_CASE(pending_payout_is_zero_before_any_payouts)
{
    auto post = create_post(initdelegate).in_block();
    post.vote(alice).in_block();

    auto d = _api.get_content(post.author(), post.permlink());

    BOOST_CHECK_EQUAL(d.pending_payout_scr, asset::from_string("0.000000000 SCR"));
    BOOST_CHECK_EQUAL(d.pending_payout_sp, asset::from_string("0.000000000 SP"));

    generate_blocks(post.cashout_time());

    d = _api.get_content(post.author(), post.permlink());

    BOOST_CHECK_EQUAL(d.pending_payout_scr, asset::from_string("0.000000000 SCR"));
    BOOST_CHECK_EQUAL(d.pending_payout_sp, asset::from_string("0.000000000 SP"));
}

SCORUM_TEST_CASE(check_pending_payout_after_first_payout)
{
    actor(initdelegate)
        .create_budget("permlink", asset::from_string("5.000000000 SCR"), db.head_block_time() + fc::days(30));

    // first payout
    {
        auto post = create_post(initdelegate).in_block();

        generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL);
        post.vote(alice).in_block();

        generate_blocks(post.cashout_time());
    }

    auto post2 = create_post(initdelegate).in_block();

    generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL);
    post2.vote(alice).in_block();

    generate_blocks(post2.cashout_time() - fc::minutes(1));

    auto d = _api.get_content(post2.author(), post2.permlink());

    BOOST_CHECK_EQUAL(d.pending_payout_scr, asset::from_string("0.000000005 SCR"));
    BOOST_CHECK_EQUAL(d.pending_payout_sp, asset::from_string("0.000000438 SP"));
}
BOOST_AUTO_TEST_SUITE_END()
