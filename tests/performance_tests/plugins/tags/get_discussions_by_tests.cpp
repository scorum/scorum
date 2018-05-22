#include "database_default_integration.hpp"
#include <scorum/tags/tags_api_objects.hpp>
#include <scorum/tags/tags_api.hpp>
#include <scorum/app/api_context.hpp>
#include <scorum/common_api/config.hpp>
#include <boost/test/unit_test.hpp>
#include <fc/filesystem.hpp>
#include <graphene/utilities/tempdir.hpp>
#include <chrono>

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;
using namespace scorum::tags;

using namespace database_fixture;

struct tag_perf_fixture : public database_fixture::database_trx_integration_fixture
{
    api_context _api_ctx;
    scorum::tags::tags_api _api;
    Actor alice;
    Actor bob;

    tag_perf_fixture()
        : _api_ctx(app, TAGS_API_NAME, std::make_shared<api_session_data>())
        , _api(_api_ctx)
        , alice("alice")
        , bob("bob")
    {
        init_plugin<scorum::tags::tags_plugin>();

        open_database();

        actor(initdelegate).create_account(alice);
    }

    virtual void open_database_impl(const genesis_state_type& genesis) override
    {
        if (!data_dir)
        {
            auto shared_file_size_4gb = 1024ul * 1024ul * 1024ul * 4ul;

            data_dir = fc::temp_directory(graphene::utilities::temp_directory_path());
            db.open(data_dir->path(), data_dir->path(), shared_file_size_4gb, chainbase::database::read_write, genesis);
            genesis_state = genesis;
        }
    }

    std::string next_permlink()
    {
        static int permlink_no = 0;
        permlink_no++;

        return boost::lexical_cast<std::string>(permlink_no);
    }
};

BOOST_FIXTURE_TEST_SUITE(get_discussions_performance_tests, tag_perf_fixture)

SCORUM_TEST_CASE(check_100000_posts_under_60ms)
{
    comment_operation op;
    op.author = alice.name;
    op.body = "body";
    op.parent_permlink = "category";
    op.json_metadata = R"({"tags":["A","B","C","D","E"]})";

    BOOST_TEST_MESSAGE("Preparing 100000 posts. It will take about 6 min...");

    uint32_t tags_count = 5 + 1; // A,B,C,D,E + <empty tag>
    uint32_t it_count = 100000;
    for (uint32_t j = 0; j < it_count; j++)
    {
        op.permlink = next_permlink();

        push_operation(op, alice.private_key, false);
        generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL + fc::seconds(SCORUM_BLOCK_INTERVAL),
                        true);
    }

    auto size = db.get_index<tag_index, by_comment>().size();
    BOOST_REQUIRE_EQUAL(size, it_count * tags_count);

    auto t1 = std::chrono::steady_clock::now();

    api::discussion_query q;
    q.tags = { "A", "B", "C", "D" };
    q.tags_logical_and = true;
    q.limit = 100;
    auto posts = _api.get_discussions_by_created(q);

    auto t2 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    BOOST_TEST_MESSAGE("get_discussions_by_created' time: " << ms << "ms");

    BOOST_CHECK(std::is_sorted(posts.begin(), posts.end(), [](const api::discussion& lhs, const api::discussion& rhs) {
        return lhs.created > rhs.created;
    }));
    BOOST_CHECK_LE(ms, 60);
}

BOOST_AUTO_TEST_SUITE_END()
