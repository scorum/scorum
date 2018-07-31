#include <memory>
#include <boost/test/unit_test.hpp>

#include <scorum/chain/database_exceptions.hpp>
#include <scorum/chain/database/block_tasks/comments_cashout_impl.hpp>

#include <scorum/chain/services/comment_statistic.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account_blogging_statistic.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/hardfork_property.hpp>

#include <scorum/protocol/block.hpp>
#include <hippomocks.h>
#include "defines.hpp"
#include "object_wrapper.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::chain::database_ns;
using namespace scorum::protocol;

namespace {
void increase_balance(const account_object& account, const asset& amount)
{
    const_cast<account_object&>(account).balance += amount;
}

const asset create_scorumpower(const account_object& account, const asset& amount)
{
    const_cast<account_object&>(account).scorumpower += amount;
    return account.scorumpower;
}

struct pay_for_comments_fixture : public shared_memory_fixture
{
    template <typename TService> using get_by_comment_id_ptr = void (TService::*)(const comment_id_type&);
    template <typename TService>
    using update_ptr
        = void (TService::*)(const typename TService::object_type&, const typename TService::modifier_type&);

    MockRepository mocks;
    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();
    comment_statistic_scr_service_i* scr_stats_service = mocks.Mock<comment_statistic_scr_service_i>();
    comment_statistic_sp_service_i* sp_stats_service = mocks.Mock<comment_statistic_sp_service_i>();
    dynamic_global_property_service_i* dyn_props_service = mocks.Mock<dynamic_global_property_service_i>();
    account_service_i* acc_service = mocks.Mock<account_service_i>();
    account_blogging_statistic_service_i* acc_blog_stats_service = mocks.Mock<account_blogging_statistic_service_i>();
    comment_service_i* comment_service = mocks.Mock<comment_service_i>();
    comment_vote_service_i* comment_vote_service = mocks.Mock<comment_vote_service_i>();
    hardfork_property_service_i* hardfork_service = mocks.Mock<hardfork_property_service_i>();
    database_virtual_operations_emmiter_i* virt_op_emitter = mocks.Mock<database_virtual_operations_emmiter_i>();

    std::shared_ptr<block_task_context> ctx;

    pay_for_comments_fixture()
    {
        mocks.OnCall(services, data_service_factory_i::comment_statistic_scr_service).ReturnByRef(*scr_stats_service);
        mocks.OnCall(services, data_service_factory_i::comment_statistic_sp_service).ReturnByRef(*sp_stats_service);
        mocks.OnCall(services, data_service_factory_i::dynamic_global_property_service).ReturnByRef(*dyn_props_service);
        mocks.OnCall(services, data_service_factory_i::account_service).ReturnByRef(*acc_service);
        mocks.OnCall(services, data_service_factory_i::account_blogging_statistic_service)
            .ReturnByRef(*acc_blog_stats_service);
        mocks.OnCall(services, data_service_factory_i::comment_service).ReturnByRef(*comment_service);
        mocks.OnCall(services, data_service_factory_i::comment_vote_service).ReturnByRef(*comment_vote_service);
        mocks.OnCall(services, data_service_factory_i::hardfork_property_service).ReturnByRef(*hardfork_service);
        mocks.OnCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);

        block_info empty_info;
        ctx = std::make_shared<block_task_context>(*services, *virt_op_emitter, 1u, empty_info);
    }

    std::vector<comment_object> create_comments()
    {
        /*
         * alice/p
         * ----bob/c1
         * --------alice/c2
         * ------------alice/c3
         */

        auto post = create_object<comment_object>(shm, [&](comment_object& c) {
            c.author = "alice";
            c.permlink = "p";
            c.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
            c.parent_permlink = "category";
            c.id = 0;
            c.depth = 0;
        });

        auto comment1 = create_object<comment_object>(shm, [&](comment_object& c) {
            c.author = "bob";
            c.permlink = "c1";
            c.parent_author = "alice";
            c.parent_permlink = "p";
            c.id = 1;
            c.depth = 1;
        });

        auto comment2 = create_object<comment_object>(shm, [&](comment_object& c) {
            c.author = "alice";
            c.permlink = "c2";
            c.parent_author = "bob";
            c.parent_permlink = "c1";
            c.id = 2;
            c.depth = 2;
        });

        auto comment3 = create_object<comment_object>(shm, [&](comment_object& c) {
            c.author = "alice";
            c.permlink = "c3";
            c.parent_author = "alice";
            c.parent_permlink = "c2";
            c.id = 3;
            c.depth = 3;
        });

        return { post, comment1, comment2, comment3 };
    }

    void mock_do_nothing()
    {
        // clang-format off
        mocks.OnCallOverload(comment_service, (update_ptr<comment_service_i>) &comment_service_i::update);
        mocks.OnCallOverload(sp_stats_service, (update_ptr<comment_statistic_sp_service_i>) &comment_statistic_sp_service_i::update);
        mocks.OnCallOverload(sp_stats_service, (get_by_comment_id_ptr<comment_statistic_sp_service_i>) &comment_statistic_sp_service_i::get);
        mocks.OnCallOverload(scr_stats_service, (update_ptr<comment_statistic_scr_service_i>) &comment_statistic_scr_service_i::update);
        mocks.OnCallOverload(scr_stats_service, (get_by_comment_id_ptr<comment_statistic_scr_service_i>) &comment_statistic_scr_service_i::get);
        mocks.OnCall(acc_blog_stats_service, account_blogging_statistic_service_i::increase_posting_rewards);
        auto obj = create_object<account_blogging_statistic_object>(shm);
        mocks.OnCall(acc_blog_stats_service, account_blogging_statistic_service_i::obtain).ReturnByRef(obj);
        mocks.OnCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);
        // clang-format on
    }

    std::pair<account_object, account_object>
    pay_comments(const std::vector<std::reference_wrapper<const comment_object>>& comment_refs,
                 const std::vector<asset>& rewards)
    {
        using get_acc_ptr
            = const comment_object& (comment_service_i::*)(const account_name_type&, const std::string&)const;

        auto alice_acc = create_object<account_object>(shm, [](account_object& acc) { acc.name = "alice"; });
        auto bob_acc = create_object<account_object>(shm, [](account_object& acc) { acc.name = "bob"; });

        mock_do_nothing();
        // 'comments' already contains all required posts/comments so we don't care which comment we should return here
        mocks.OnCallOverload(comment_service, (get_acc_ptr)&comment_service_i::get).ReturnByRef(comment_refs[0].get());
        mocks.OnCall(acc_service, account_service_i::get_account).With("alice").ReturnByRef(alice_acc);
        mocks.OnCall(acc_service, account_service_i::get_account).With("bob").ReturnByRef(bob_acc);
        mocks.OnCall(acc_service, account_service_i::create_scorumpower).Do(create_scorumpower);
        mocks.OnCall(acc_service, account_service_i::increase_balance).Do(increase_balance);

        process_comments_cashout_impl cashout(*ctx);
        cashout.pay_for_comments(comment_refs, rewards);

        return std::make_pair(alice_acc, bob_acc);
    }
};

BOOST_FIXTURE_TEST_SUITE(pay_for_comments_tests, pay_for_comments_fixture)

BOOST_AUTO_TEST_CASE(check_same_author_comments_sp_reward_before_hardfork_order1)
{
    auto comments = create_comments();

    std::vector<std::reference_wrapper<const comment_object>> comment_refs
        = { std::cref(comments[3]), std::cref(comments[1]), std::cref(comments[2]), std::cref(comments[0]) };
    std::vector<asset> fund_rewards = { ASSET_SP(40), ASSET_SP(60), ASSET_SP(100), ASSET_SP(120) };

    mocks.OnCall(hardfork_service, hardfork_property_service_i::has_hardfork).With(1).Return(false);

    auto accs_pair = pay_comments(comment_refs, fund_rewards);

    auto& alice_acc = accs_pair.first;
    auto& bob_acc = accs_pair.second;

    // clang-format off
    /*
     * We had a bug before hardfork in 'parent-comments-distribution-algorithm'
     * rewards (from children comments and from fund) where being associated with parent author and not with parent comment.
     *
     * Here is an algorithm of how payments where processed before hardfork:
     *
     * post | [alice] | fund_reward = 40
     * --comment1 | [bob] | fund_reward = 60
     * ----comment2 | [alice] | fund_reward = 40
     * ------comment3 | [alice] | fund_reward = 40
     *
     * 1. comment_3 payment:
     *   - 40/2 from fund goes to alice account
     *   - 40/2 from fund adds to alice 'wallet' i.e. alice 'wallet' equals 40/2 for now
     * 2. comment_2 payment:
     *   - 40/2 from fund goes to alice account
     *   - 1/2 of alice's 'wallet' (40/4) goes to alice account
     *   - 40/2 from fund adds to parent author's 'wallet' i.e. bob 'wallet' equals 40/2 for now
     *   - 1/2 of alice's 'wallet' (40/4) adds to bob 'wallet' i.e. bob 'wallet' equals 40/2 + 40/4 for now
     * 3. comment_1 payment:
     *   - 60/2 from fund goes to bob account
     *   - 1/2 of bob's 'wallet' (40/4 + 40/8) goes to alice account
     *   - 60/2 from fund adds to alice 'wallet' i.e. alice 'wallet' equals 40/2 + 60/2 for now
     *   - 1/2 of bob's 'wallet' (40/4 + 40/8) adds to alice 'wallet' i.e. alice 'wallet' equals 40/2 + 60/2 + 40/4 + 40/8 for now
     * 4. post payment:
     *   - 40 from fund goes to alice account
     *   - alice wallet (40/2 + 60/2 + 40/4 + 40/8) goes to alice account
     *
     * Note: alice has 'fund_reward' 40 in all comments
     */

    BOOST_CHECK_EQUAL(alice_acc.scorumpower.amount, (40 + 40/2 + 60/2 + 40/4 + 40/8) + (40/2 + 40/4) + 40/2);
    BOOST_CHECK_EQUAL(bob_acc.scorumpower.amount, 60/2 + 40/4 + 40/8);
    // clang-format on
}

BOOST_AUTO_TEST_CASE(check_same_author_comments_sp_reward_before_hardfork_order2)
{
    auto comments = create_comments();

    std::vector<std::reference_wrapper<const comment_object>> comment_refs
        = { std::cref(comments[0]), std::cref(comments[3]), std::cref(comments[1]), std::cref(comments[2]) };
    std::vector<asset> fund_rewards = { ASSET_SP(120), ASSET_SP(40), ASSET_SP(60), ASSET_SP(100) };

    mocks.OnCall(hardfork_service, hardfork_property_service_i::has_hardfork).With(1).Return(false);

    auto accs_pair = pay_comments(comment_refs, fund_rewards);

    auto& alice_acc = accs_pair.first;
    auto& bob_acc = accs_pair.second;

    /*
     * Same algorithm as in 'check_same_author_comments_sp_reward_before_hardfork_order1'
     *
     * Note: alice has 'fund_reward' 120 in all comments
     */

    BOOST_CHECK_EQUAL(alice_acc.scorumpower.amount,
                      (120 + 120 / 2 + 60 / 2 + 120 / 4 + 120 / 8) + (120 / 2 + 120 / 4) + 120 / 2);
    BOOST_CHECK_EQUAL(bob_acc.scorumpower.amount, 60 / 2 + 120 / 4 + 120 / 8);
}

BOOST_AUTO_TEST_CASE(check_same_author_comments_sp_reward_after_hardfork)
{
    auto comments = create_comments();

    std::vector<std::reference_wrapper<const comment_object>> comment_refs
        = { std::cref(comments[3]), std::cref(comments[1]), std::cref(comments[2]), std::cref(comments[0]) };
    std::vector<asset> fund_rewards = { ASSET_SP(40), ASSET_SP(60), ASSET_SP(100), ASSET_SP(120) };

    mocks.OnCall(hardfork_service, hardfork_property_service_i::has_hardfork).With(1).Return(true);

    auto accs_pair = pay_comments(comment_refs, fund_rewards);

    auto& alice_acc = accs_pair.first;
    auto& bob_acc = accs_pair.second;

    auto post_reward_from_fund = 120u;
    auto post_reward_from_comment_1 = 60u / 2;
    auto post_reward_from_comment_2 = 100u / 4;
    auto post_reward_from_comment_3 = 40u / 8;
    auto comment_1_reward_from_fund = 60u / 2;
    auto comment_1_reward_from_comment_2 = 100u / 4;
    auto comment_1_reward_from_comment_3 = 40u / 8;
    auto comment_2_reward_from_fund = 100u / 2;
    auto comment_2_reward_from_comment_3 = 40u / 4;
    auto comment_3_reward_from_fund = 40u / 2;

    // clang-format off
    BOOST_REQUIRE_EQUAL(alice_acc.scorumpower.amount + bob_acc.scorumpower.amount, 120u + 60u + 100u + 40u);
    // (post + 0.5*c1 + 0.25c2 + 0.125c3) [post reward] + (0.5c2 + 0.25c3) [c2 reward] + (0.5c3) [c3 reward]
    BOOST_CHECK_EQUAL(alice_acc.scorumpower.amount,
                      (post_reward_from_fund + post_reward_from_comment_1 + post_reward_from_comment_2 + post_reward_from_comment_3) +
                      (comment_2_reward_from_fund + comment_2_reward_from_comment_3) +
                      (comment_3_reward_from_fund));
    // 0.5c1 + 0.25c2 + 0.125c3 [c1 reward]
    BOOST_CHECK_EQUAL(bob_acc.scorumpower.amount,
                      comment_1_reward_from_fund + comment_1_reward_from_comment_2 + comment_1_reward_from_comment_3);
    // clang-format on
}

BOOST_AUTO_TEST_CASE(check_same_author_comments_scr_reward_after_hardfork_order1)
{
    auto comments = create_comments();

    std::vector<std::reference_wrapper<const comment_object>> comment_refs
        = { std::cref(comments[1]), std::cref(comments[0]), std::cref(comments[2]), std::cref(comments[3]) };
    std::vector<asset> fund_rewards = { ASSET_SCR(60), ASSET_SCR(120), ASSET_SCR(100), ASSET_SCR(40) };

    mocks.OnCall(hardfork_service, hardfork_property_service_i::has_hardfork).With(1).Return(true);

    auto accs_pair = pay_comments(comment_refs, fund_rewards);

    auto& alice_acc = accs_pair.first;
    auto& bob_acc = accs_pair.second;

    auto post_reward_from_fund = 120u;
    auto post_reward_from_comment_1 = 60u / 2;
    auto post_reward_from_comment_2 = 100u / 4;
    auto post_reward_from_comment_3 = 40u / 8;
    auto comment_1_reward_from_fund = 60u / 2;
    auto comment_1_reward_from_comment_2 = 100u / 4;
    auto comment_1_reward_from_comment_3 = 40u / 8;
    auto comment_2_reward_from_fund = 100u / 2;
    auto comment_2_reward_from_comment_3 = 40u / 4;
    auto comment_3_reward_from_fund = 40u / 2;

    // clang-format off
    BOOST_REQUIRE_EQUAL(alice_acc.balance.amount + bob_acc.balance.amount, 120u + 60u + 100u + 40u);
    // (post + 0.5*c1 + 0.25c2 + 0.125c3) [post reward] + (0.5c2 + 0.25c3) [c2 reward] + (0.5c3) [c3 reward]
    BOOST_CHECK_EQUAL(alice_acc.balance.amount,
                      (post_reward_from_fund + post_reward_from_comment_1 + post_reward_from_comment_2 + post_reward_from_comment_3) +
                      (comment_2_reward_from_fund + comment_2_reward_from_comment_3) +
                      (comment_3_reward_from_fund));
    // 0.5c1 + 0.25c2 + 0.125c3 [c1 reward]
    BOOST_CHECK_EQUAL(bob_acc.balance.amount,
                      comment_1_reward_from_fund + comment_1_reward_from_comment_2 + comment_1_reward_from_comment_3);
    // clang-format on
}

BOOST_AUTO_TEST_CASE(check_same_author_comments_scr_reward_after_hardfork_order2)
{
    auto comments = create_comments();

    std::vector<std::reference_wrapper<const comment_object>> comment_refs
        = { std::cref(comments[3]), std::cref(comments[1]), std::cref(comments[2]), std::cref(comments[0]) };
    std::vector<asset> fund_rewards = { ASSET_SCR(40), ASSET_SCR(60), ASSET_SCR(100), ASSET_SCR(120) };

    mocks.OnCall(hardfork_service, hardfork_property_service_i::has_hardfork).With(1).Return(true);

    auto accs_pair = pay_comments(comment_refs, fund_rewards);

    auto& alice_acc = accs_pair.first;
    auto& bob_acc = accs_pair.second;

    auto post_reward_from_fund = 120u;
    auto post_reward_from_comment_1 = 60u / 2;
    auto post_reward_from_comment_2 = 100u / 4;
    auto post_reward_from_comment_3 = 40u / 8;
    auto comment_1_reward_from_fund = 60u / 2;
    auto comment_1_reward_from_comment_2 = 100u / 4;
    auto comment_1_reward_from_comment_3 = 40u / 8;
    auto comment_2_reward_from_fund = 100u / 2;
    auto comment_2_reward_from_comment_3 = 40u / 4;
    auto comment_3_reward_from_fund = 40u / 2;

    // clang-format off
    BOOST_REQUIRE_EQUAL(alice_acc.balance.amount + bob_acc.balance.amount, 120u + 60u + 100u + 40u);
    // (post + 0.5*c1 + 0.25c2 + 0.125c3) [post reward] + (0.5c2 + 0.25c3) [c2 reward] + (0.5c3) [c3 reward]
    BOOST_CHECK_EQUAL(alice_acc.balance.amount,
                      (post_reward_from_fund + post_reward_from_comment_1 + post_reward_from_comment_2 + post_reward_from_comment_3) +
                      (comment_2_reward_from_fund + comment_2_reward_from_comment_3) +
                      (comment_3_reward_from_fund));
    // 0.5c1 + 0.25c2 + 0.125c3 [c1 reward]
    BOOST_CHECK_EQUAL(bob_acc.balance.amount,
                      comment_1_reward_from_fund + comment_1_reward_from_comment_2 + comment_1_reward_from_comment_3);
    // clang-format on
}

BOOST_AUTO_TEST_SUITE_END()
}
