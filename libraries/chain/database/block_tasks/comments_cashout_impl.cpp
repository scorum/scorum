#include <scorum/chain/database/block_tasks/comments_cashout_impl.hpp>
#include <boost/range/combine.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

process_comments_cashout_impl::process_comments_cashout_impl(block_task_context& ctx)
    : _ctx(ctx)
    , dgp_service(ctx.services().dynamic_global_property_service())
    , account_service(ctx.services().account_service())
    , account_blogging_statistic_service(ctx.services().account_blogging_statistic_service())
    , comment_service(ctx.services().comment_service())
    , comment_statistic_scr_service(ctx.services().comment_statistic_scr_service())
    , comment_statistic_sp_service(ctx.services().comment_statistic_sp_service())
    , comment_vote_service(ctx.services().comment_vote_service())
    , hardfork_service(ctx.services().hardfork_property_service())
{
}

void process_comments_cashout_impl::close_comment_payout(const comment_object& comment)
{
    comment_service.update(comment, [&](comment_object& c) {

        // TODO: remove abs_* fields (it is used only in tests)
        c.children_abs_rshares = 0;
        c.abs_rshares = 0;

        // to reset next payout for curators
        c.total_vote_weight = 0;

        c.cashout_time = fc::time_point_sec::maximum();
        c.last_payout = dgp_service.head_block_time();
    });

    _ctx.push_virtual_operation(comment_payout_update_operation(comment.author, fc::to_string(comment.permlink)));

#ifdef CLEAR_VOTES
    auto comment_votes = comment_vote_service.get_by_comment(comment.id);
    for (const comment_vote_object& vote : comment_votes)
    {
        comment_vote_service.remove(vote);
    }
#endif
}

fc::uint128_t process_comments_cashout_impl::get_total_claims(const comment_refs_type& comments,
                                                              curve_id reward_curve,
                                                              fc::uint128_t recent_claims) const
{
    shares_vector_type total_rshares;
    for (const comment_object& comment : comments)
    {
        total_rshares.push_back(comment.net_rshares);
    }

    fc::uint128_t total_claims = rewards_math::calculate_total_claims(recent_claims, reward_curve, total_rshares);

    return total_claims;
}

template <typename TFundService>
void process_comments_cashout_impl::reward(TFundService& fund_service, const comment_refs_type& comments)
{
    const auto& fund = fund_service.get();
    if (fund.activity_reward_balance.amount < 1 || comments.empty())
        return;

    auto total_claims = get_total_claims(comments, fund.author_reward_curve, fund.recent_claims);

    auto fund_rewards
        = calculate_comments_payout(comments, fund.activity_reward_balance, total_claims, fund.author_reward_curve);

    auto total_reward = hardfork_service.has_hardfork(SCORUM_HARDFORK_0_3)
        ? pay_for_comments(comments, fund_rewards)
        : pay_for_comments_legacy(comments, fund_rewards);

    fund_service.update([&](typename TFundService::object_type& rfo) {
        rfo.recent_claims = total_claims;
        rfo.activity_reward_balance -= total_reward;
        rfo.last_update = dgp_service.head_block_time();
    });
}

asset process_comments_cashout_impl::pay_for_comments(const comment_refs_type& comments,
                                                      const std::vector<asset>& fund_rewards)
{
    namespace math = rewards_math;

    FC_ASSERT(comments.size() == fund_rewards.size(), "comments count and comments' rewards count should be equal");
    FC_ASSERT(!fund_rewards.empty(), "collection cannot be empty");

    auto reward_symbol = fund_rewards[0].symbol();
    auto total_claimed_reward = asset(0, reward_symbol);

    for (const auto& item : boost::combine(comments, fund_rewards))
    {
        const auto& comment = item.get<0>().get();
        const auto& fund_reward = item.get<1>();

        auto rewards = pay_curators(comment, fund_reward);
        auto beneficiaries_reward = pay_beneficiaries(comment, rewards.author_reward);

        auto curators_reward = rewards.curators_reward;
        auto author_reward = rewards.author_reward - beneficiaries_reward;

        const auto& author = account_service.get_account(comment.author);
        pay_account(author, author_reward);

        auto claimed_reward = curators_reward + author_reward + beneficiaries_reward;
        total_claimed_reward += claimed_reward;

        comment_service.update(comment, [&](comment_object& c) { c.last_payout = dgp_service.head_block_time(); });
        if (author_reward.amount > 0)
            comment_service.set_rewarded_flag(comment);

        _ctx.push_virtual_operation(
            author_reward_operation(comment.author, fc::to_string(comment.permlink), author_reward));

        // clang-format off
        _ctx.push_virtual_operation(comment_reward_operation(
                comment.author,
                fc::to_string(comment.permlink),
                fund_reward,
                claimed_reward,
                author_reward,
                curators_reward,
                asset(0, reward_symbol),
                asset(0, reward_symbol),
                beneficiaries_reward));

        accumulate_statistic(comment,
                             author,
                             fund_reward,
                             author_reward,
                             curators_reward,
                             asset(0, reward_symbol),
                             asset(0, reward_symbol),
                             beneficiaries_reward,
                             reward_symbol);
        // clang-format on
    }

    return total_claimed_reward;
}

asset process_comments_cashout_impl::pay_for_comments_legacy(const comment_refs_type& comments,
                                                             const std::vector<asset>& fund_rewards)
{
    FC_ASSERT(comments.size() == fund_rewards.size(), "comments count and comments' rewards count should be equal");
    FC_ASSERT(fund_rewards.size() > 0, "collection cannot be empty");

    asset_symbol_type reward_symbol = fund_rewards[0].symbol();

    struct comment_reward
    {
        share_type fund; // reward accrued from fund
        share_type commenting; // reward accrued from children comments
    };
    struct comment_key
    {
        account_name_type author;
        fc::shared_string permlink;
    };
    auto less = [](const comment_key& l, const comment_key& r) {
        return std::tie(l.author, l.permlink) < std::tie(r.author, r.permlink);
    };

    std::map<comment_key, comment_reward, decltype(less)> comment_rewards(less);
    for (auto i = 0u; i < comments.size(); ++i)
    {
        comment_rewards.emplace(comment_key{ comments[i].get().author, get_permlink(comments[i].get().permlink) },
                                comment_reward{ fund_rewards[i].amount, 0 });
    }

    asset total_reward = asset(0, reward_symbol);

    // newest, with bigger depth comments first
    comment_refs_type comments_with_parents = collect_parents(comments);

    for (const comment_object& comment : comments_with_parents)
    {
        const auto& reward = comment_rewards[comment_key{ comment.author, get_permlink(comment.permlink) }];

        asset fund_reward = asset(reward.fund, reward_symbol);
        asset parent_payout_value = asset(reward.commenting, reward_symbol);

        comment_payout_result payout_result = pay_for_comment_legacy(comment, fund_reward, parent_payout_value);

        total_reward += payout_result.total_claimed_reward;

        // save payout for the parent comment
        comment_rewards[comment_key{ comment.parent_author, get_permlink(comment.parent_permlink) }].commenting
            += payout_result.parent_comment_reward.amount;
    }

    return total_reward;
}

process_comments_cashout_impl::comment_payout_result process_comments_cashout_impl::pay_for_comment_legacy(
    const comment_object& comment, const asset& fund_reward, const asset& payout_from_children)
{
    try
    {
        auto reward_symbol = fund_reward.symbol();
        comment_payout_result payout_result{ asset(0, reward_symbol), asset(0, reward_symbol) };
        if (fund_reward.amount < 1 && payout_from_children.amount < 1)
            return payout_result;

        auto rewards = pay_curators(comment, fund_reward);
        auto author_reward = rewards.author_reward;
        auto curators_reward = rewards.curators_reward;

        auto payout_to_parent = asset(0, reward_symbol);
        if (comment.depth != 0)
        {
            auto fraction = utils::make_fraction(SCORUM_PARENT_COMMENT_REWARD_PERCENT, SCORUM_100_PERCENT);
            payout_to_parent = (author_reward + payout_from_children) * fraction;
        }

        author_reward = (author_reward + payout_from_children) - payout_to_parent;

        auto beneficiaries_reward = pay_beneficiaries(comment, author_reward);
        author_reward -= beneficiaries_reward;

        const auto& author = account_service.get_account(comment.author);
        pay_account(author, author_reward);

        payout_result.total_claimed_reward = curators_reward + author_reward + beneficiaries_reward;
        payout_result.parent_comment_reward = payout_to_parent;

        comment_service.update(comment, [&](comment_object& c) { c.last_payout = dgp_service.head_block_time(); });
        if (author_reward.amount > 0 || payout_from_children.amount > 0)
            comment_service.set_rewarded_flag(comment);

        _ctx.push_virtual_operation(
            author_reward_operation(comment.author, fc::to_string(comment.permlink), author_reward));

        // clang-format off
        _ctx.push_virtual_operation(comment_reward_operation(
                             comment.author,
                             fc::to_string(comment.permlink),
                             fund_reward,
                             payout_result.total_claimed_reward,
                             author_reward,
                             curators_reward,
                             payout_from_children,
                             payout_to_parent,
                             beneficiaries_reward));

        accumulate_statistic(comment,
                             author,
                             fund_reward,
                             author_reward,
                             curators_reward,
                             payout_from_children,
                             payout_to_parent,
                             beneficiaries_reward,
                             reward_symbol);
        // clang-format on

        return payout_result;
    }
    FC_CAPTURE_AND_RETHROW((comment))
}

process_comments_cashout_impl::curators_author_rewards
process_comments_cashout_impl::pay_curators(const comment_object& comment, const asset& fund_reward)
{
    try
    {
        auto reward_symbol = fund_reward.symbol();
        auto potential_reward = fund_reward * utils::make_fraction(SCORUM_CURATION_REWARD_PERCENT, SCORUM_100_PERCENT);

        if (!comment.allow_curation_rewards)
            return { asset(0, reward_symbol), fund_reward - potential_reward };
        else if (comment.total_vote_weight <= 0)
            return { asset(0, reward_symbol), fund_reward };

        auto rewarded = asset(0, reward_symbol);
        auto comment_votes = comment_vote_service.get_by_comment_weight_voter(comment.id);
        for (const comment_vote_object& vote : comment_votes)
        {
            auto claim = potential_reward * utils::make_fraction(vote.weight, comment.total_vote_weight);
            if (claim.amount > 0)
            {
                rewarded += claim;

                const auto& voter = account_service.get(vote.voter);
                pay_account(voter, claim);

                _ctx.push_virtual_operation(
                    curation_reward_operation(voter.name, claim, comment.author, fc::to_string(comment.permlink)));

                accumulate_statistic(voter, claim);
            }
        }

        return { rewarded, fund_reward - rewarded };
    }
    FC_CAPTURE_AND_RETHROW((comment.author)(comment.permlink)(fund_reward))
}

asset process_comments_cashout_impl::pay_beneficiaries(const comment_object& comment, const asset& author_reward)
{
    auto reward_symbol = author_reward.symbol();

    auto beneficiaries_reward = asset(0, reward_symbol);
    for (auto& beneficiary : comment.beneficiaries)
    {
        auto beneficiary_reward = author_reward * utils::make_fraction(beneficiary.weight, SCORUM_100_PERCENT);
        pay_account(account_service.get_account(beneficiary.account), beneficiary_reward);
        beneficiaries_reward += beneficiary_reward;

        _ctx.push_virtual_operation(comment_benefficiary_reward_operation(
            beneficiary.account, comment.author, fc::to_string(comment.permlink), beneficiary_reward));
    }

    return beneficiaries_reward;
}

void process_comments_cashout_impl::pay_account(const account_object& recipient, const asset& reward)
{
    if (SCORUM_SYMBOL == reward.symbol())
    {
        account_service.increase_balance(recipient, reward);
    }
    else if (SP_SYMBOL == reward.symbol())
    {
        account_service.create_scorumpower(recipient, reward);
    }
}

std::vector<asset> process_comments_cashout_impl::calculate_comments_payout(const comment_refs_type& comments,
                                                                            const asset& reward_fund_balance,
                                                                            fc::uint128_t total_claims,
                                                                            curve_id reward_curve) const
{
    std::vector<asset> rewards;
    rewards.reserve(comments.size());

    for (const comment_object& comment : comments)
    {
        share_type payout = rewards_math::calculate_payout(
            comment.net_rshares, total_claims, reward_fund_balance.amount, reward_curve,
            comment.max_accepted_payout.amount, SCORUM_MIN_COMMENT_PAYOUT_SHARE);

        rewards.emplace_back(payout, reward_fund_balance.symbol());
    }

    return rewards;
}

void process_comments_cashout_impl::accumulate_statistic(const comment_object& comment,
                                                         const account_object& author,
                                                         const asset& fund_reward,
                                                         const asset& author_payout,
                                                         const asset& curation_payout,
                                                         const asset& payout_from_children,
                                                         const asset& payout_to_parent,
                                                         const asset& beneficiary_payout,
                                                         asset_symbol_type reward_symbol)
{
    FC_ASSERT(author_payout.symbol() == reward_symbol);
    FC_ASSERT(curation_payout.symbol() == reward_symbol);
    FC_ASSERT(beneficiary_payout.symbol() == reward_symbol);

    asset total_payout = author_payout;
    total_payout += curation_payout;
    total_payout += beneficiary_payout;

    // clang-format off
    if (SCORUM_SYMBOL == reward_symbol)
    {
        accumulate_comment_statistic(comment_statistic_scr_service, comment,
                                     fund_reward,
                                     total_payout,
                                     author_payout,
                                     curation_payout,
                                     payout_from_children,
                                     payout_to_parent,
                                     beneficiary_payout);
    }
    else if (SP_SYMBOL == reward_symbol)
    {
        accumulate_comment_statistic(comment_statistic_sp_service, comment,
                                     fund_reward,
                                     total_payout,
                                     author_payout,
                                     curation_payout,
                                     payout_from_children,
                                     payout_to_parent,
                                     beneficiary_payout);
    }
// clang-format on

#ifndef IS_LOW_MEM
    {
        const auto& author_stat = account_blogging_statistic_service.obtain(author.id);
        account_blogging_statistic_service.increase_posting_rewards(author_stat, author_payout);
    }
#endif
}

void process_comments_cashout_impl::accumulate_statistic(const account_object& voter, const asset& curation_payout)
{
#ifndef IS_LOW_MEM
    const auto& voter_stat = account_blogging_statistic_service.obtain(voter.id);
    account_blogging_statistic_service.increase_curation_rewards(voter_stat, curation_payout);
#endif
}

comment_refs_type process_comments_cashout_impl::collect_parents(const comment_refs_type& comments)
{
    struct by_depth_greater
    {
        bool operator()(const comment_object& lhs, const comment_object& rhs)
        {
            return std::tie(lhs.depth, lhs.id) > std::tie(rhs.depth, rhs.id);
        }
    };

    using comment_refs_set = std::set<comment_refs_type::value_type, by_depth_greater>;
    comment_refs_set ordered_comments(comments.begin(), comments.end());

    // 'ordered_comments' set is sorted by depth in desc order.
    for (auto it = ordered_comments.begin(); it != ordered_comments.end() && it->get().depth != 0; ++it)
    {
        const comment_object& comment = it->get();

        const auto& parent_comment = comment_service.get(comment.parent_author, fc::to_string(comment.parent_permlink));

        // insert parent if it doesn't exist or do nothing if exists. Insertable parent will be always 'after' current
        // comment because of the set ordering
        ordered_comments.insert(parent_comment);
    }

    comment_refs_type comments_with_parents(ordered_comments.begin(), ordered_comments.end());

    return comments_with_parents;
}

fc::shared_string process_comments_cashout_impl::get_permlink(const fc::shared_string& str) const
{
    if (hardfork_service.has_hardfork(SCORUM_HARDFORK_0_1))
        return str;
    else
        return fc::shared_string("", str.get_allocator());
}

// Explicit template instantiation
// clang-format off
template void process_comments_cashout_impl::reward<content_reward_fund_scr_service_i>(content_reward_fund_scr_service_i&, const comment_refs_type&);
template void process_comments_cashout_impl::reward<content_reward_fund_sp_service_i>(content_reward_fund_sp_service_i&, const comment_refs_type&);
template void process_comments_cashout_impl::reward<content_fifa_world_cup_2018_bounty_reward_fund_service_i>( content_fifa_world_cup_2018_bounty_reward_fund_service_i&, const comment_refs_type&);
// clang-format on
}
}
}
