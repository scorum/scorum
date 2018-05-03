#include <boost/test/unit_test.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/database_exceptions.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_statistic.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

#include "database_default_integration.hpp"
#include "actoractions.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <string>
#include <map>
#include <set>

using namespace database_fixture;
struct comment
{
    account_name_type author;
    uint32_t block_num;
    std::vector<comment> children;
};

struct ordered_comment_operation
{
    comment_operation chain_op;
    uint32_t block_num;
    uint32_t order; // this member is used to keep order of comments which are posted within same block
};

struct comments_hierarchy_reward_fixture : public database_default_integration_fixture
{
    const share_type account_initial_sp = 1e5;

    std::map<account_name_type, share_type> account_initial_balances;

    account_service_i& account_service;
    comment_service_i& comment_service;
    comment_statistic_sp_service_i& comment_statistic_sp_service;

    comments_hierarchy_reward_fixture()
        : account_service(db.obtain_service<dbs_account>())
        , comment_service(db.obtain_service<dbs_comment>())
        , comment_statistic_sp_service(db.obtain_service<dbs_comment_statistic_sp>())
    {
    }

    std::vector<ordered_comment_operation>
    to_flatten_operations(const comment& cmt, const account_name_type& parent_author, uint32_t order = 0)
    {
        std::vector<ordered_comment_operation> ops;

        ordered_comment_operation op;
        op.chain_op.author = cmt.author;
        op.chain_op.body = cmt.author + "-body";
        op.chain_op.permlink = cmt.author + "-permlink";
        op.chain_op.title = cmt.author + "-title";
        op.chain_op.parent_author = parent_author;
        op.chain_op.parent_permlink = parent_author + "-permlink";
        op.block_num = cmt.block_num;
        op.order = order;

        for (auto& child_cmt : cmt.children)
        {
            auto children_ops = to_flatten_operations(child_cmt, cmt.author, order + 1);
            std::copy(children_ops.begin(), children_ops.end(), std::back_inserter(ops));
        }

        ops.push_back(op);

        return ops;
    }

    void create_actors(const std::vector<ordered_comment_operation>& ops)
    {
        for (const auto& op : ops)
        {
            auto author_name = op.chain_op.author;
            auto voter_name = boost::replace_all_copy((std::string)op.chain_op.author, "user", "voter");

            Actor commenter(author_name);
            Actor voter(voter_name);

            actor(initdelegate).create_account(commenter);
            actor(initdelegate).create_account(voter);

            actor(initdelegate).give_sp(commenter, account_initial_sp.value);
            actor(initdelegate).give_sp(voter, account_initial_sp.value);

            account_initial_balances[author_name] = get_total_tokens(author_name);
            account_initial_balances[voter_name] = get_total_tokens(voter_name);
        }
    }

    void post_comments(const std::vector<ordered_comment_operation>& ops, const uint32_t delay_between_blocks)
    {
        // group by 'block num'
        std::map<uint32_t, std::vector<std::reference_wrapper<const ordered_comment_operation>>> groupped_ops;
        for (const auto& op : ops)
            groupped_ops[op.block_num].push_back(std::cref(op));

        for (auto& group : groupped_ops)
        {
            // children comments should be posted after its parents within same block
            std::sort(group.second.begin(), group.second.end(),
                      [](const ordered_comment_operation& lhs, const ordered_comment_operation& rhs) {
                          return lhs.order < rhs.order;
                      });

            signed_transaction tx;
            std::transform(group.second.begin(), group.second.end(), std::back_inserter(tx.operations),
                           [](const ordered_comment_operation& op) { return op.chain_op; });

            tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
            tx.sign(initdelegate.private_key, db.get_chain_id());
            db.push_transaction(tx, default_skip);

            generate_blocks(db.head_block_time() + delay_between_blocks);
        }
    }

    void vote_comments(const std::vector<ordered_comment_operation>& ops)
    {
        signed_transaction tx;

        for (const ordered_comment_operation& op : ops)
        {
            vote_operation vote_op;
            vote_op.author = op.chain_op.author;
            vote_op.permlink = op.chain_op.permlink;
            vote_op.voter = boost::replace_all_copy((std::string)op.chain_op.author, "user", "voter");
            vote_op.weight = 100;

            tx.operations.push_back(vote_op);
        }

        tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
        tx.sign(initdelegate.private_key, db.get_chain_id());

        db.push_transaction(tx, default_skip);
    }

    const comment_statistic_sp_object& get_statistics(const account_name_type& account)
    {
        const comment_object& c = comment_service.get(account, account + "-permlink");

        const comment_statistic_sp_object& statistics = comment_statistic_sp_service.get(c.id);

        return statistics;
    }

    share_type get_total_tokens(account_name_type account)
    {
        const account_object& acc = account_service.get_account(account);

        return acc.balance.amount + acc.scorumpower.amount;
    }

    share_type check_comment_rewards(const comment_statistic_sp_object& stat, share_type payout_from_children)
    {
        asset user_parent_reward = (stat.comment_publication_reward - stat.curator_payout_value + payout_from_children)
            * SCORUM_PARENT_COMMENT_REWARD_PERCENT / SCORUM_100_PERCENT;
        BOOST_REQUIRE_EQUAL(user_parent_reward + stat.total_payout_value,
                            stat.comment_publication_reward + payout_from_children);

        return user_parent_reward.amount;
    }

    void check_post_rewards(const comment_statistic_sp_object& stat, share_type payout_from_children)
    {
        BOOST_REQUIRE_EQUAL(stat.total_payout_value, stat.comment_publication_reward + payout_from_children);
    }

    void check_account_balance(const account_name_type& account, share_type reward)
    {
        share_type current_balance = get_total_tokens(account);
        share_type old_balance = account_initial_balances[account];

        BOOST_REQUIRE_EQUAL(current_balance, old_balance + reward);
    }

    void check_4level_hierarchy(const comment& hierarchy);
    void check_3level_hierarchy_with_multiple_children(const comment& hierarchy);
};

void comments_hierarchy_reward_fixture::check_4level_hierarchy(const comment& hierarchy)
{
    std::vector<ordered_comment_operation> ops = to_flatten_operations(hierarchy, SCORUM_ROOT_POST_PARENT_ACCOUNT);

    create_actors(ops);
    generate_block();

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    post_comments(ops, delay);

    // wait till voters could obtain max reward
    generate_blocks(db.head_block_time() + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    vote_comments(ops);

    auto last_comment_cashout = fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS) - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS;
    // skip blocks till comments cashout
    generate_blocks(db.head_block_time() + last_comment_cashout);

    auto user30_parent_reward = check_comment_rewards(get_statistics("user30"), 0);
    auto user20_parent_reward = check_comment_rewards(get_statistics("user20"), user30_parent_reward);
    auto user10_parent_reward = check_comment_rewards(get_statistics("user10"), user20_parent_reward);

    check_post_rewards(get_statistics("user00"), user10_parent_reward);

    check_account_balance("user30", get_statistics("user30").author_payout_value.amount);
    check_account_balance("user20", get_statistics("user20").author_payout_value.amount);
    check_account_balance("user10", get_statistics("user10").author_payout_value.amount);
    check_account_balance("user00", get_statistics("user00").author_payout_value.amount);

    check_account_balance("voter30", get_statistics("user30").curator_payout_value.amount);
    check_account_balance("voter20", get_statistics("user20").curator_payout_value.amount);
    check_account_balance("voter10", get_statistics("user10").curator_payout_value.amount);
    check_account_balance("voter00", get_statistics("user00").curator_payout_value.amount);
}

void comments_hierarchy_reward_fixture::check_3level_hierarchy_with_multiple_children(const comment& hierarchy)
{
    std::vector<ordered_comment_operation> ops = to_flatten_operations(hierarchy, SCORUM_ROOT_POST_PARENT_ACCOUNT);

    create_actors(ops);
    generate_block();

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    post_comments(ops, delay);

    // wait till voters could obtain max reward
    generate_blocks(db.head_block_time() + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    vote_comments(ops);

    auto last_comment_cashout = fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS) - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS;
    // skip blocks till comments cashout
    generate_blocks(db.head_block_time() + last_comment_cashout);

    auto user20_parent_reward = check_comment_rewards(get_statistics("user20"), 0);
    auto user21_parent_reward = check_comment_rewards(get_statistics("user21"), 0);
    auto user10_parent_reward
        = check_comment_rewards(get_statistics("user10"), user20_parent_reward + user21_parent_reward);
    auto user22_parent_reward = check_comment_rewards(get_statistics("user22"), 0);
    auto user11_parent_reward = check_comment_rewards(get_statistics("user11"), user22_parent_reward);

    check_post_rewards(get_statistics("user00"), user10_parent_reward + user11_parent_reward);

    check_account_balance("user22", get_statistics("user22").author_payout_value.amount);
    check_account_balance("user21", get_statistics("user21").author_payout_value.amount);
    check_account_balance("user20", get_statistics("user20").author_payout_value.amount);
    check_account_balance("user10", get_statistics("user10").author_payout_value.amount);
    check_account_balance("user11", get_statistics("user11").author_payout_value.amount);
    check_account_balance("user00", get_statistics("user00").author_payout_value.amount);
}

BOOST_FIXTURE_TEST_SUITE(comment_hierarchy_reward_tests, comments_hierarchy_reward_fixture)

BOOST_AUTO_TEST_CASE(check_each_comment_on_diff_block)
{
    // clang-format off
    comment hierarchy =
        { "user00", 0, {
            { "user10", 1, {
                { "user20", 2, {
                    { "user30", 3, {} } }} }} } };
    // clang-format on

    check_4level_hierarchy(hierarchy);
}

BOOST_AUTO_TEST_CASE(check_several_comments_on_same_block)
{
    // clang-format off
    comment hierarchy =
        { "user00", 0, {
            { "user10", 1, {
                { "user20", 1, {
                    { "user30", 2, {} } }} }} } };
    // clang-format on

    check_4level_hierarchy(hierarchy);
}

BOOST_AUTO_TEST_CASE(check_multiple_children_each_level_on_diff_block)
{
    // clang-format off
    comment hierarchy =
        { "user00", 0, {
            { "user10", 1, {
                { "user20", 2, {}},
                { "user21", 2, {}} }},
            { "user11", 1, {
                { "user22", 2, {}} }} }};
    // clang-format on

    check_3level_hierarchy_with_multiple_children(hierarchy);
}

BOOST_AUTO_TEST_CASE(check_multiple_children_with_parent_within_same_block)
{
    // clang-format off
    comment hierarchy =
        { "user00", 0, {
            { "user10", 1, {
                { "user20", 1, {}},
                { "user21", 1, {}} }},
            { "user11", 1, {
                { "user22", 2, {}} }} }};
    // clang-format on

    check_3level_hierarchy_with_multiple_children(hierarchy);
}

BOOST_AUTO_TEST_CASE(check_beneficiares_payouts)
{
    // clang-format off
    comment hierarchy =
        { "user00", 0, {
            { "user10", 1, {
                { "user20", 2, {
                    { "user30", 3, {} } }} }} } };
    // clang-format on

    std::vector<ordered_comment_operation> ops = to_flatten_operations(hierarchy, SCORUM_ROOT_POST_PARENT_ACCOUNT);

    create_actors(ops);
    generate_block();

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    post_comments(ops, delay);

    Actor beneficiar("beneficiar");
    actor(initdelegate).create_account(beneficiar);
    actor(initdelegate).give_sp(beneficiar, account_initial_sp.value);

    account_initial_balances["beneficiar"] = get_total_tokens("beneficiar");

    comment_options_operation op;
    op.author = "user10";
    op.permlink = "user10-permlink";
    op.allow_curation_rewards = true;
    comment_payout_beneficiaries b;
    b.beneficiaries.push_back(beneficiary_route_type(account_name_type("beneficiar"), 50 * SCORUM_1_PERCENT));
    op.extensions.insert(b);
    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
    tx.sign(initdelegate.private_key, db.get_chain_id());
    db.push_transaction(tx, default_skip);
    generate_block();

    // wait till voters could obtain max reward
    generate_blocks(db.head_block_time() + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    vote_comments(ops);

    auto last_comment_cashout = fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS) - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS;

    // skip blocks till comments cashout
    generate_blocks(db.head_block_time() + last_comment_cashout);

    auto user30_parent_reward = check_comment_rewards(get_statistics("user30"), 0);
    auto user20_parent_reward = check_comment_rewards(get_statistics("user20"), user30_parent_reward);
    auto user10_parent_reward = check_comment_rewards(get_statistics("user10"), user20_parent_reward);

    check_post_rewards(get_statistics("user00"), user10_parent_reward);

    check_account_balance("user30", get_statistics("user30").author_payout_value.amount);
    check_account_balance("user20", get_statistics("user20").author_payout_value.amount);
    check_account_balance("user10", get_statistics("user10").author_payout_value.amount);
    check_account_balance("user00", get_statistics("user00").author_payout_value.amount);

    check_account_balance("beneficiar", get_statistics("user10").beneficiary_payout_value.amount);
}

BOOST_AUTO_TEST_CASE(check_each_comment_on_diff_level_two_cashouts)
{
    // clang-format off
    comment hierarchy =
        { "user00", 0, {
            { "user10", 1, {
                { "user20", 2, {
                    { "user30", 3, {} } }} }} } };
    // clang-format on

    std::vector<ordered_comment_operation> ops = to_flatten_operations(hierarchy, SCORUM_ROOT_POST_PARENT_ACCOUNT);

    create_actors(ops);
    generate_block();

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    post_comments(ops, delay);

    // wait till voters could obtain max reward
    generate_blocks(db.head_block_time() + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    vote_comments(ops);

    // perform 'user00' and 'user10' cashout
    {
        auto user10_comment_cashout = fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS) - fc::seconds(delay * 3)
            - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS;

        // skip blocks till user10 comment cashout. user00 & user10 will be rewarded
        generate_blocks(db.head_block_time() + user10_comment_cashout);

        auto user10_parent_reward = check_comment_rewards(get_statistics("user10"), 0);

        check_post_rewards(get_statistics("user00"), user10_parent_reward);

        check_account_balance("user10", get_statistics("user10").author_payout_value.amount);
        check_account_balance("user00", get_statistics("user00").author_payout_value.amount);
    }

    // perform 'user20' and 'user30' cashout
    {
        auto head_time = db.head_block_time();
        // max delay which do not exceed 'user20' comment cashout (considering that 'generate_blocks' calls apply_block
        // twice: for the first block and for the last block in the defined interval)
        auto reward_accumulation_delay = delay - 2 * SCORUM_BLOCK_INTERVAL;
        // accumulate reward fund
        generate_blocks(head_time + reward_accumulation_delay, false);

        auto last_comment_cashout = delay * 2;
        // skip blocks till last comment cashout. all users will be rewarded but user00 & user10 only with 'commenting'
        // reward from children comments
        generate_blocks(head_time + last_comment_cashout);

        auto user30_parent_reward = check_comment_rewards(get_statistics("user30"), 0);
        auto user20_parent_reward = check_comment_rewards(get_statistics("user20"), user30_parent_reward);
        auto user10_parent_reward = check_comment_rewards(get_statistics("user10"), user20_parent_reward);

        check_post_rewards(get_statistics("user00"), user10_parent_reward);

        check_account_balance("user30", get_statistics("user30").author_payout_value.amount);
        check_account_balance("user20", get_statistics("user20").author_payout_value.amount);
        check_account_balance("user10", get_statistics("user10").author_payout_value.amount);
        check_account_balance("user00", get_statistics("user00").author_payout_value.amount);
    }
}

BOOST_AUTO_TEST_SUITE_END()
