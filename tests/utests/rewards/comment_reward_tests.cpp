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

struct pay_for_comments_fixture : public shared_memory_fixture
{
    template <typename TService>
    using get_by_comment_id_ptr = const typename TService::object_type& (TService::*)(const comment_id_type&)const;
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
        // clang-format off
        : alice_acc(create_object<account_object>(shm, [](account_object& acc) { acc.name = "alice"; acc.id = 0; }))
        , bob_acc(create_object<account_object>(shm, [](account_object& acc) { acc.name = "bob"; acc.id = 1; }))
        , sam_acc(create_object<account_object>(shm, [](account_object& acc) { acc.name = "sam"; acc.id = 2; }))
        , dave_acc(create_object<account_object>(shm, [](account_object& acc) { acc.name = "dave"; acc.id = 3; }))
    // clang-format on
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
            c.allow_curation_rewards = true;
            c.total_vote_weight = 0;
        });

        auto comment1 = create_object<comment_object>(shm, [&](comment_object& c) {
            c.author = "bob";
            c.permlink = "c1";
            c.parent_author = "alice";
            c.parent_permlink = "p";
            c.id = 1;
            c.depth = 1;
            c.allow_curation_rewards = true;
            c.total_vote_weight = 0;
        });

        auto comment2 = create_object<comment_object>(shm, [&](comment_object& c) {
            c.author = "alice";
            c.permlink = "c2";
            c.parent_author = "bob";
            c.parent_permlink = "c1";
            c.id = 2;
            c.depth = 2;
            c.allow_curation_rewards = true;
            c.total_vote_weight = 0;
        });

        auto comment3 = create_object<comment_object>(shm, [&](comment_object& c) {
            c.author = "alice";
            c.permlink = "c3";
            c.parent_author = "alice";
            c.parent_permlink = "c2";
            c.id = 3;
            c.depth = 3;
            c.allow_curation_rewards = true;
            c.total_vote_weight = 0;
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
        mocks.OnCall(acc_blog_stats_service, account_blogging_statistic_service_i::obtain);
        mocks.OnCall(acc_blog_stats_service, account_blogging_statistic_service_i::increase_posting_rewards);
        mocks.OnCall(acc_blog_stats_service, account_blogging_statistic_service_i::increase_curation_rewards);
        static auto obj = create_object<account_blogging_statistic_object>(shm);
        mocks.OnCall(acc_blog_stats_service, account_blogging_statistic_service_i::obtain).ReturnByRef(obj);
        mocks.OnCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);
        // clang-format on
    }

    void pay_comments(const std::vector<std::reference_wrapper<const comment_object>>& comment_refs,
                      const std::vector<asset>& rewards)
    {
        using get_ptr = const account_object& (account_service_i::*)(const account_id_type&)const;
        using get_acc_ptr
            = const comment_object& (comment_service_i::*)(const account_name_type&, const std::string&)const;
        using inc_acc_balance_ptr = void (account_service_i::*)(const account_object&, const asset&);

        mock_do_nothing();
        // 'comments' already contains all required posts/comments so we don't care which comment we should return here
        mocks.OnCallOverload(comment_service, (get_acc_ptr)&comment_service_i::get).ReturnByRef(comment_refs[0].get());
        mocks.OnCall(comment_service, comment_service_i::set_rewarded_flag);
        mocks.OnCall(acc_service, account_service_i::get_account).With("alice").ReturnByRef(alice_acc);
        mocks.OnCall(acc_service, account_service_i::get_account).With("bob").ReturnByRef(bob_acc);
        mocks.OnCall(acc_service, account_service_i::get_account).With("sam").ReturnByRef(sam_acc);
        mocks.OnCall(acc_service, account_service_i::get_account).With("dave").ReturnByRef(dave_acc);
        mocks.OnCallOverload(acc_service, (get_ptr)&account_service_i::get).With(2).ReturnByRef(sam_acc);
        mocks.OnCallOverload(acc_service, (get_ptr)&account_service_i::get).With(3).ReturnByRef(dave_acc);
        mocks.OnCall(acc_service, account_service_i::create_scorumpower)
            .Do([](const account_object& account, const asset& amount) -> const asset {
                const_cast<account_object&>(account).scorumpower += amount;
                return account.scorumpower;
            });
        mocks.OnCallOverload(acc_service, (inc_acc_balance_ptr)&account_service_i::increase_balance)
            .Do([](const account_object& account, const asset& amount) {
                const_cast<account_object&>(account).balance += amount;
            });

        process_comments_cashout_impl cashout(*ctx);
        cashout.pay_for_comments(comment_refs, rewards);
    }

    account_object alice_acc;
    account_object bob_acc;
    account_object sam_acc;
    account_object dave_acc;
};

BOOST_FIXTURE_TEST_SUITE(pay_for_comments_tests, pay_for_comments_fixture)

BOOST_AUTO_TEST_CASE(check_author_rewards_no_curators_rewards)
{
    auto comments = create_comments();

    std::vector<std::reference_wrapper<const comment_object>> comment_refs
        = { std::cref(comments[3]), std::cref(comments[1]), std::cref(comments[2]), std::cref(comments[0]) };
    std::vector<asset> fund_rewards = { ASSET_SP(40), ASSET_SP(60), ASSET_SP(100), ASSET_SP(120) };

    pay_comments(comment_refs, fund_rewards);

    BOOST_CHECK_EQUAL(alice_acc.scorumpower.amount, 120 + 100 + 40);
    BOOST_CHECK_EQUAL(bob_acc.scorumpower.amount, 60);
}

BOOST_AUTO_TEST_CASE(check_author_rewards_and_curators_rewards)
{
    auto comments = create_comments();
    comments[0].total_vote_weight = 5000; // sam & dave vote
    comments[1].total_vote_weight = 2500; // sam vote
    comments[2].total_vote_weight = 2500; // dave vote
    comments[3].total_vote_weight = 2500; // sam vote

    auto comment_0_sam_vote = create_object<comment_vote_object>(shm, [](comment_vote_object& v) {
        v.comment = 0;
        v.weight = 2500;
        v.voter = 2; // sam
    });
    auto comment_0_dave_vote = create_object<comment_vote_object>(shm, [](comment_vote_object& v) {
        v.comment = 0;
        v.weight = 2500;
        v.voter = 3; // dave
    });
    auto comment_1_sam_vote = create_object<comment_vote_object>(shm, [](comment_vote_object& v) {
        v.comment = 1;
        v.weight = 2500;
        v.voter = 2; // sam
    });
    auto comment_2_dave_vote = create_object<comment_vote_object>(shm, [](comment_vote_object& v) {
        v.comment = 2;
        v.weight = 2500;
        v.voter = 3; // dave
    });
    auto comment_3_sam_vote = create_object<comment_vote_object>(shm, [](comment_vote_object& v) {
        v.comment = 3;
        v.weight = 2500;
        v.voter = 2; // sam
    });

    using votes_vec_t = std::vector<std::reference_wrapper<const comment_vote_object>>;

    mocks.OnCall(comment_vote_service, comment_vote_service_i::get_by_comment_weight_voter)
        .With(0)
        .Return(votes_vec_t{ std::cref(comment_0_sam_vote), std::cref(comment_0_dave_vote) });
    mocks.OnCall(comment_vote_service, comment_vote_service_i::get_by_comment_weight_voter)
        .With(1)
        .Return(votes_vec_t{ std::cref(comment_1_sam_vote) });
    mocks.OnCall(comment_vote_service, comment_vote_service_i::get_by_comment_weight_voter)
        .With(2)
        .Return(votes_vec_t{ std::cref(comment_2_dave_vote) });
    mocks.OnCall(comment_vote_service, comment_vote_service_i::get_by_comment_weight_voter)
        .With(3)
        .Return(votes_vec_t{ std::cref(comment_3_sam_vote) });

    std::vector<std::reference_wrapper<const comment_object>> comment_refs
        = { std::cref(comments[0]), std::cref(comments[1]), std::cref(comments[2]), std::cref(comments[3]) };
    std::vector<asset> fund_rewards = { ASSET_SP(120), ASSET_SP(60), ASSET_SP(100), ASSET_SP(40) };

    pay_comments(comment_refs, fund_rewards);

    // author rewards
    BOOST_CHECK_EQUAL(alice_acc.scorumpower.amount, 120 * 3 / 4 + 100 * 3 / 4 + 40 * 3 / 4);
    BOOST_CHECK_EQUAL(bob_acc.scorumpower.amount, 60 * 3 / 4);

    // curators rewards
    BOOST_CHECK_EQUAL(sam_acc.scorumpower.amount, 120 * 1 / 8 + 60 * 1 / 4 + 40 * 1 / 4);
    BOOST_CHECK_EQUAL(dave_acc.scorumpower.amount, 120 * 1 / 8 + 100 * 1 / 4);
}

BOOST_AUTO_TEST_CASE(check_author_rewards_and_beneficiaries_rewards)
{
    auto comments = create_comments();
    comments[0].beneficiaries.emplace_back("sam", 2500);
    comments[0].beneficiaries.emplace_back("dave", 5000);
    comments[1].beneficiaries.emplace_back("sam", 2500);

    std::vector<std::reference_wrapper<const comment_object>> comment_refs
        = { std::cref(comments[0]), std::cref(comments[1]) };
    std::vector<asset> fund_rewards = { ASSET_SP(120), ASSET_SP(60) };

    pay_comments(comment_refs, fund_rewards);

    // authors rewards
    BOOST_CHECK_EQUAL(alice_acc.scorumpower.amount, 120 * 1 / 4);
    BOOST_CHECK_EQUAL(bob_acc.scorumpower.amount, 60 * 3 / 4);

    // beneficiaries rewards
    BOOST_CHECK_EQUAL(sam_acc.scorumpower.amount, 120 * 1 / 4 + 60 * 1 / 4);
    BOOST_CHECK_EQUAL(dave_acc.scorumpower.amount, 120 * 1 / 2);
}

BOOST_AUTO_TEST_CASE(one_satoshi_fund_reward_test)
{
    auto comments = create_comments();
    comments[0].total_vote_weight = 2500; // sam vote

    auto comment_0_sam_vote = create_object<comment_vote_object>(shm, [](comment_vote_object& v) {
        v.comment = 0;
        v.weight = 2500;
        v.voter = 2; // sam
    });

    using votes_vec_t = std::vector<std::reference_wrapper<const comment_vote_object>>;

    mocks.OnCall(comment_vote_service, comment_vote_service_i::get_by_comment_weight_voter)
        .With(0)
        .Return(votes_vec_t{ std::cref(comment_0_sam_vote) });

    std::vector<std::reference_wrapper<const comment_object>> comment_refs = { std::cref(comments[0]) };
    std::vector<asset> fund_rewards = { ASSET_SP(1) };

    pay_comments(comment_refs, fund_rewards);

    // 1 satoshi goes to author
    BOOST_CHECK_EQUAL(alice_acc.scorumpower.amount, 1);
    BOOST_CHECK_EQUAL(sam_acc.scorumpower.amount, 0);
}

BOOST_AUTO_TEST_CASE(zero_fund_reward_no_effect_test)
{
    auto comments = create_comments();
    comments[0].total_vote_weight = 2500; // sam vote

    auto comment_0_sam_vote = create_object<comment_vote_object>(shm, [](comment_vote_object& v) {
        v.comment = 0;
        v.weight = 2500;
        v.voter = 2; // sam
    });

    using votes_vec_t = std::vector<std::reference_wrapper<const comment_vote_object>>;

    mocks.OnCall(comment_vote_service, comment_vote_service_i::get_by_comment_weight_voter)
        .With(0)
        .Return(votes_vec_t{ std::cref(comment_0_sam_vote) });

    std::vector<std::reference_wrapper<const comment_object>> comment_refs = { std::cref(comments[0]) };
    std::vector<asset> fund_rewards = { ASSET_SP(0) };

    pay_comments(comment_refs, fund_rewards);

    // 1 satoshi goes to author
    BOOST_CHECK_EQUAL(alice_acc.scorumpower.amount, 0);
    BOOST_CHECK_EQUAL(sam_acc.scorumpower.amount, 0);
}

BOOST_AUTO_TEST_CASE(different_comments_and_rewards_count_should_throw)
{
    auto comment = create_object<comment_object>(shm, [&](comment_object&) {});

    std::vector<std::reference_wrapper<const comment_object>> comment_refs = { std::cref(comment) };
    std::vector<asset> fund_rewards = { ASSET_SP(100), ASSET_SP(200) };

    process_comments_cashout_impl cashout(*ctx);
    BOOST_CHECK_THROW(cashout.pay_for_comments(comment_refs, fund_rewards), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(comments_and_rewards_are_empty_should_throw)
{
    std::vector<std::reference_wrapper<const comment_object>> comment_refs = {};
    std::vector<asset> fund_rewards = {};

    process_comments_cashout_impl cashout(*ctx);
    BOOST_CHECK_THROW(cashout.pay_for_comments(comment_refs, fund_rewards), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()
}
