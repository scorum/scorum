#include <boost/test/unit_test.hpp>

#include "database_blog_integration.hpp"

#include <scorum/snapshot/snapshot_helper.hpp>
#include <scorum/snapshot/snapshot_plugin.hpp>
#include <scorum/chain/services/comment.hpp>

#include <graphene/utilities/tempdir.hpp>
#include <fc/filesystem.hpp>

#include <boost/program_options.hpp>

namespace snapshot_tests {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

using namespace database_fixture;

namespace po = boost::program_options;

struct snapshot_fixture : public database_blog_integration_fixture
{
    snapshot_fixture()
        : alice("alice")
        , bob("bob")
        , sam("sam")
        , dprops_service(db.dynamic_global_property_service())
        , comment_service(db.comment_service())
    {
        plugin = app.register_plugin<scorum::snapshot::snapshot_plugin>();
        app.enable_plugin(plugin->plugin_name());

        po::options_description cfg;
        app.set_program_options(desc, cfg);
    }

    void open_database(const po::variables_map& options = po::variables_map())
    {
        plugin->plugin_initialize(options);
        plugin->plugin_startup();

        database_integration_fixture::open_database();

        create_account(alice);
        create_account(bob);
        create_account(sam);

        generate_block();
    }

    void replay_database(const fc::temp_directory& dir,
                         const po::variables_map& options,
                         const uint32_t replay_from_snapshot_number)
    {
        plugin->plugin_initialize(options);
        plugin->plugin_startup();

        db.reindex(dir.path(), dir.path(), TEST_SHARED_MEM_SIZE_10MB,
                   scorum::to_underlying(database::open_flags::read_write), genesis_state, replay_from_snapshot_number);
    }

    void create_account(Actor& a, const int feed_amount = 99000)
    {
        actor(initdelegate).create_account(a);
        actor(initdelegate).give_scr(a, feed_amount);
        actor(initdelegate).give_sp(a, feed_amount);
    }

    po::variables_map parse_input(const std::vector<std::string>& input)
    {
        try
        {
            // Parse mocked up input.
            po::variables_map vm;
            po::store(po::command_line_parser(input).options(desc).run(), vm);
            po::notify(vm);
            return vm;
        }
        FC_LOG_AND_RETHROW()
    }

    struct test_case_1
    {
        comment_op alice_post;
        comment_op sam_post;
    };

    //  'Alice' create post
    //  'Sam' create post
    //  'Sam' votes for 'Alice' post
    test_case_1 create_test_case_1()
    {
        auto p1 = create_post(alice)
                      .set_json(
                          R"({"domains": ["chain_tests"], "categories": ["snapshot_tests"], "tags": ["test"]})")
                      .in_block();
        auto p2 = create_post(sam)
                      .set_json(R"({"domains": ["chain_tests"], "categories": ["snapshot_tests"], "tags": ["test"]})")
                      .in_block();

        p1.vote(sam).in_block();

        return { p1, p2 };
    }

    struct test_case_2
    {
        comment_op bob_post;
    };

    //  'Bob' create post
    //  'Sam' delete own post
    //  'Bob' votes for 'Alice' post
    test_case_2 create_test_case_2(test_case_1& prev_case)
    {
        auto p = create_post(bob)
                     .set_json(R"({"domains": ["chain_tests"], "categories": ["snapshot_tests"], "tags": ["test"]})")
                     .in_block();

        prev_case.sam_post.remove();
        prev_case.alice_post.vote(bob).in_block();

        return { p };
    }

    std::shared_ptr<scorum::snapshot::snapshot_plugin> plugin;
    po::options_description desc;

    Actor alice;
    Actor bob;
    Actor sam;
    dynamic_global_property_service_i& dprops_service;
    comment_service_i& comment_service;
};

struct snapshot_reopen_fixture
{
    snapshot_reopen_fixture()
    {
        reset();
    }

    void reset()
    {
        fixture.reset(new snapshot_fixture());
    }

    std::unique_ptr<snapshot_fixture> fixture;
};

BOOST_AUTO_TEST_SUITE(snapshot_tests)

BOOST_FIXTURE_TEST_CASE(dont_throw_if_snapshot_with_no_snapshot_dir, snapshot_fixture)
{
    open_database();

    BOOST_CHECK_NO_THROW(app.sigusr1());

    BOOST_CHECK_NO_THROW(generate_block());
}

BOOST_FIXTURE_TEST_CASE(save_snapshot_check, snapshot_fixture)
{
    auto dir = fc::temp_directory(graphene::utilities::temp_directory_path());
    auto snapshot_save_dir_tpl = R"(--snapshot-save-dir="${dir}")";
    std::vector<std::string> input = { fc::format_string(
        snapshot_save_dir_tpl, fc::mutable_variant_object()("dir", dir.path().generic_string())) };

    wdump((input));

    open_database(parse_input(input));

    BOOST_CHECK_NO_THROW(app.sigusr1());

    BOOST_CHECK(!fc::exists(snapshot::create_snapshot_path(dprops_service, dir.path())));

    BOOST_CHECK_NO_THROW(generate_block());

    BOOST_CHECK(fc::exists(snapshot::create_snapshot_path(dprops_service, dir.path())));
}

BOOST_FIXTURE_TEST_CASE(save_and_load_snapshot_check, snapshot_reopen_fixture)
{
    static const int snapshot_number = 20;

    SCORUM_MESSAGE("-- Create database state and save in snapshot file");

    auto dir = fc::temp_directory(graphene::utilities::temp_directory_path());
    {
        auto snapshot_save_dir_tpl = R"(--snapshot-save-dir="${dir}")";
        auto save_snapshot_number_tpl = R"(--save-snapshot-number=${number})";
        std::vector<std::string> input
            = { fc::format_string(snapshot_save_dir_tpl,
                                  fc::mutable_variant_object()("dir", dir.path().generic_string())),
                fc::format_string(save_snapshot_number_tpl, fc::mutable_variant_object()("number", snapshot_number)) };

        wdump((input));

        fixture->open_database(fixture->parse_input(input));
    }

    auto test_case_1 = fixture->create_test_case_1();
    auto test_case_2 = fixture->create_test_case_2(test_case_1);

    BOOST_REQUIRE(
        fixture->comment_service.is_exists(test_case_1.alice_post.author(), test_case_1.alice_post.permlink()));
    BOOST_REQUIRE(!fixture->comment_service.is_exists(test_case_1.sam_post.author(), test_case_1.sam_post.permlink()));
    BOOST_REQUIRE(fixture->comment_service.is_exists(test_case_2.bob_post.author(), test_case_2.bob_post.permlink()));

    BOOST_REQUIRE_GT(snapshot_number, fixture->db.head_block_num());

    BOOST_CHECK_NO_THROW(fixture->generate_blocks(
        fixture->db.head_block_time()
            + fc::seconds(SCORUM_BLOCK_INTERVAL * (snapshot_number - fixture->db.head_block_num())),
        false));

    auto snapshot_path = snapshot::create_snapshot_path(fixture->dprops_service, dir.path());

    BOOST_REQUIRE(fc::exists(snapshot_path));

    fixture->generate_blocks(SCORUM_MAX_WITNESSES_LIMIT);

    SCORUM_MESSAGE("-- Open new database");

    auto db_dir = std::move(fixture->data_dir);
    reset();

    {
        auto load_snapshot_file_tpl = R"(--load-snapshot-file="${file}")";
        std::vector<std::string> input = { fc::format_string(
            load_snapshot_file_tpl, fc::mutable_variant_object()("file", snapshot_path.generic_string())) };

        wdump((input));

        SCORUM_MESSAGE("-- Sanpshot is restoring full state");

        fixture->replay_database(*db_dir, fixture->parse_input(input), snapshot_number);
    }

    SCORUM_MESSAGE("-- Check that 'Alice' and 'Bob' posts exist and 'Sam' post is deleted");

    BOOST_CHECK(fixture->comment_service.is_exists(test_case_1.alice_post.author(), test_case_1.alice_post.permlink()));
    BOOST_CHECK(!fixture->comment_service.is_exists(test_case_1.sam_post.author(), test_case_1.sam_post.permlink()));
    BOOST_CHECK(fixture->comment_service.is_exists(test_case_2.bob_post.author(), test_case_2.bob_post.permlink()));
}

BOOST_AUTO_TEST_SUITE_END()
}
