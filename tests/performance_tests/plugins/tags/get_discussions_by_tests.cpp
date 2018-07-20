#include "database_default_integration.hpp"
#include <scorum/tags/tags_api_objects.hpp>
#include <scorum/tags/tags_api.hpp>
#include <scorum/app/api_context.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/common_api/config_api.hpp>
#include <boost/test/unit_test.hpp>
#include <fc/filesystem.hpp>
#include <graphene/utilities/tempdir.hpp>
#include <chrono>
#include <random>

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
            auto shared_file_size_4gb = 1024 * 1024 * 1024 * 4ul;

            data_dir = fc::temp_directory(graphene::utilities::temp_directory_path());
            db.open(data_dir->path(), data_dir->path(), shared_file_size_4gb, chainbase::database::read_write, genesis);
            genesis_state = genesis;
        }
    }

    void check_N_posts_under_M_ms(uint32_t posts_count, uint32_t expected_ms)
    {
        auto acc_name = "alice";
        auto tags = { "a", "b", "c", "d", "e", "f", "g", "h", "" };

        auto& acc_service = db.obtain_service<dbs_account>();
        auto alice_id = acc_service.get_account(acc_name).id;

        std::random_device device;
        std::mt19937 generator(device());
        std::uniform_int_distribution<> distr(3000, posts_count + 3000);

        for (uint32_t i = 0; i < posts_count; i++)
        {
            const auto& comment = db.create<comment_object>([&](comment_object& c) {
                c.author = acc_name;
                fc::from_string(c.permlink, boost::lexical_cast<std::string>(i));
            });
            db.create<comment_statistic_scr_object>([&](comment_statistic_scr_object& o) { o.comment = comment.id; });
            db.create<comment_statistic_sp_object>([&](comment_statistic_sp_object& o) { o.comment = comment.id; });

            for (auto& t : tags)
            {
                db.create<tag_object>([&](tag_object& obj) {
                    obj.tag = t;
                    obj.comment = comment.id;
                    obj.created = fc::time_point_sec(distr(generator));
                    obj.author = alice_id;
                });
            }
        }

        auto size = db.get_index<tag_index, by_comment>().size();
        BOOST_REQUIRE_EQUAL(size, posts_count * tags.size());

        auto t1 = std::chrono::steady_clock::now();

        api::discussion_query q;
        q.tags = { "A", "B", "C", "D" };
        q.tags_logical_and = true;
        q.limit = 100;
        auto posts = _api.get_discussions_by_created(q);

        auto t2 = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        BOOST_TEST_MESSAGE("get_discussions_by_created' time: " << ms << "ms");

        BOOST_CHECK(
            std::is_sorted(posts.begin(), posts.end(), [](const api::discussion& lhs, const api::discussion& rhs) {
                return lhs.created > rhs.created;
            }));
        BOOST_CHECK_LE(ms, expected_ms);
    }
};

BOOST_FIXTURE_TEST_SUITE(get_discussions_performance_tests, tag_perf_fixture)

SCORUM_TEST_CASE(check_100000_posts_under_100ms)
{
    BOOST_TEST_MESSAGE("Checking 100'000 posts should be under 100ms");

    check_N_posts_under_M_ms(100000, 100);
}

SCORUM_TEST_CASE(check_1000000_posts_under_1000ms)
{
    BOOST_TEST_MESSAGE("Checking 1'000'000 posts should be under 1000ms");

    check_N_posts_under_M_ms(1000000, 1000);
}

BOOST_AUTO_TEST_SUITE_END()
