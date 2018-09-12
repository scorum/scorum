#include <boost/test/unit_test.hpp>

#include "database_blog_integration.hpp"

namespace legacy_virtual_op_tests {

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;

using namespace database_fixture;

struct legacy_virtual_op_fixture : public database_blog_integration_fixture
{
protected:
    legacy_virtual_op_fixture(uint32_t hardfork)
        : _hardfork(hardfork)
        , alice("alice")
        , bob("bob")
        , sam("sam")
    {
        open_database();

        create_actor(alice);
        create_actor(bob);
        create_actor(sam);
    }

    void create_actor(const Actor& a)
    {
        actor(initdelegate).create_account(a);
        actor(initdelegate).give_sp(a, 1e5);
    }

    virtual void open_database_impl(const genesis_state_type& genesis) override
    {
        database_integration_fixture::open_database_impl(genesis);

        db.set_hardfork(_hardfork);
    }

    uint32_t _hardfork = SCORUM_NUM_HARDFORKS;

public:
    struct test_case_1
    {
        comment_op alice_post;
    };

    // test case #1:
    //--------------
    //  'Alice' create post
    //  'Bob' vote for 'Alice' post
    //  'Sam' vote for 'Alice'' comment
    test_case_1 create_test_case_1()
    {
        auto p = create_post(alice)
                     .set_json(
                         R"({"domains": ["chain_tests"], "categories": ["legacy_virtual_op_tests"], "tags": ["test"]})")
                     .in_block();

        generate_blocks(db.head_block_time() + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

        if (db.has_hardfork(SCORUM_HARDFORK_0_2))
        {
            p.vote(bob, SCORUM_PERCENT(100)).in_block();
            p.vote(sam, SCORUM_PERCENT(100)).in_block();
        }
        else
        {
            p.vote(bob, 100).in_block();
            p.vote(sam, 100).in_block();
        }

        return { p };
    }

    void subscribe_vops()
    {
        checked_legacy.clear();
        checked_new.clear();

        db.post_apply_operation.connect([&](const operation_notification& note) {
            note.op.weak_visit(
                [&](const active_sp_holders_reward_legacy_operation& op) {
                    for (auto it = op.rewarded.begin(); it != op.rewarded.end(); ++it)
                    {
                        auto inserted = checked_legacy.insert(std::make_pair(it->first, it->second));
                        if (inserted.second)
                            inserted.first->second += it->second;
                    }
                },
                [&](const active_sp_holders_reward_operation& op) {
                    auto inserted = checked_new.insert(std::make_pair(op.sp_holder, op.reward));
                    if (inserted.second)
                        inserted.first->second += op.reward;
                });
        });
    }

    using rewarded_type = std::map<account_name_type, asset>;

    rewarded_type checked_legacy;
    rewarded_type checked_new;

    Actor alice;
    Actor bob;
    Actor sam;
};

struct legacy_virtual_op_hardfork_0_1_fixture : public legacy_virtual_op_fixture
{
    legacy_virtual_op_hardfork_0_1_fixture()
        : legacy_virtual_op_fixture(SCORUM_HARDFORK_0_1)
    {
    }
};

struct legacy_virtual_op_hardfork_0_2_fixture : public legacy_virtual_op_fixture
{
    legacy_virtual_op_hardfork_0_2_fixture()
        : legacy_virtual_op_fixture(SCORUM_HARDFORK_0_2)
    {
    }
};

BOOST_AUTO_TEST_SUITE(legacy_virtual_op_tests)

BOOST_FIXTURE_TEST_CASE(active_sp_holders_reward_legacy_operation_check, legacy_virtual_op_hardfork_0_1_fixture)
{
    auto test_case = create_test_case_1();

    subscribe_vops();

    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    BOOST_CHECK_GT(checked_legacy[bob.name], ASSET_NULL_SP);
    BOOST_CHECK_GT(checked_legacy[sam.name], ASSET_NULL_SP);
    BOOST_CHECK_EQUAL(checked_legacy[alice.name], ASSET_NULL_SCR);

    BOOST_CHECK_EQUAL(checked_new[bob.name], ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(checked_new[sam.name], ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(checked_new[alice.name], ASSET_NULL_SCR);
}

BOOST_FIXTURE_TEST_CASE(active_sp_holders_reward_new_operation_check, legacy_virtual_op_hardfork_0_2_fixture)
{
    auto test_case = create_test_case_1();

    subscribe_vops();

    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    BOOST_CHECK_EQUAL(checked_legacy[bob.name], ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(checked_legacy[sam.name], ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(checked_legacy[alice.name], ASSET_NULL_SCR);

    BOOST_CHECK_GT(checked_new[bob.name], ASSET_NULL_SP);
    BOOST_CHECK_GT(checked_new[sam.name], ASSET_NULL_SP);
    BOOST_CHECK_EQUAL(checked_new[alice.name], ASSET_NULL_SCR);
}

BOOST_AUTO_TEST_SUITE_END()
}
