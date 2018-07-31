#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/max_element.hpp>
#include <boost/range/algorithm/lower_bound.hpp>
#include <boost/range/join.hpp>

#include <stack>
#include <set>

#include <scorum/protocol/types.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/witness_schedule.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/rewards_math/formulas.hpp>

#include <scorum/common_api/config.hpp>
#include <scorum/tags/tags_api_objects.hpp>
#include <scorum/tags/tags_service.hpp>

#include <scorum/utils/string_algorithm.hpp>

namespace scorum {
namespace tags {

using namespace scorum::tags::api;

template <typename Fund> asset calc_pending_payout(const discussion& d, const Fund& reward_fund_obj)
{
    share_type pending_payout_value;

    if (d.cashout_time != fc::time_point_sec::maximum() && reward_fund_obj.recent_claims > 0 && d.net_rshares > 0)
    {
        // clang-format off
        pending_payout_value = rewards_math::predict_payout(reward_fund_obj.recent_claims,
                                                            reward_fund_obj.activity_reward_balance.amount,
                                                            d.net_rshares,
                                                            reward_fund_obj.author_reward_curve,
                                                            d.max_accepted_payout.amount,
                                                            SCORUM_RECENT_RSHARES_DECAY_RATE,
                                                            SCORUM_MIN_COMMENT_PAYOUT_SHARE);
        // clang-format on
    }

    return asset(pending_payout_value, reward_fund_obj.symbol());
}

class tags_api_impl
{
public:
    using posts_crefs = std::vector<std::reference_wrapper<const tag_object>>;

    tags_api_impl(scorum::chain::database& db)
        : _db(db)
        , _services(_db)
        , _tags_service(_db)
    {
    }

    ~tags_api_impl()
    {
    }

    discussion create_discussion(const comment_object& comment) const
    {
        return discussion(comment, _services.comment_statistic_scr_service(), _services.comment_statistic_sp_service());
    }

    std::vector<api::tag_api_obj> get_trending_tags(const std::string& after_tag, uint32_t limit) const
    {
        limit = std::min(limit, uint32_t(LOOKUP_LIMIT));
        std::vector<api::tag_api_obj> result;
        result.reserve(limit);

        const auto& nidx = _db.get_index<tags::tag_stats_index>().indices().get<tags::by_tag>();

        const auto& ridx = _db.get_index<tags::tag_stats_index>().indices().get<tags::by_trending>();
        auto itr = ridx.begin();
        if (after_tag != "" && nidx.size())
        {
            auto nitr = nidx.lower_bound(after_tag);
            if (nitr == nidx.end())
                itr = ridx.end();
            else
                itr = ridx.iterator_to(*nitr);
        }

        while (itr != ridx.end() && result.size() < limit)
        {
            result.push_back(api::tag_api_obj(*itr));
            ++itr;
        }
        return result;
    }

    std::vector<std::pair<std::string, uint32_t>> get_tags_used_by_author(const std::string& author) const
    {
        const auto& acnt = _services.account_service().get_account(author);

        const auto& tidx = _db.get_index<tags::author_tag_stats_index, tags::by_author_posts_tag>();
        auto itr = tidx.lower_bound(boost::make_tuple(acnt.id, 0));
        std::vector<std::pair<std::string, uint32_t>> result;

        while (itr != tidx.end() && itr->author == acnt.id && result.size() < LOOKUP_LIMIT)
        {
            result.push_back(std::make_pair(itr->tag, itr->total_posts));
            ++itr;
        }

        return result;
    }

    std::vector<std::pair<std::string, uint32_t>> get_tags_by_category(const std::string& domain,
                                                                       const std::string& category) const
    {
        FC_ASSERT(!domain.empty(), "domain cannot be empty");
        FC_ASSERT(!category.empty(), "category cannot be empty");

        const auto& idx = _db.get_index<tags::category_stats_index, tags::by_category>();

        auto normalized_domain = utils::substring(utils::to_lower_copy(domain), 0, TAG_LENGTH_MAX);
        auto normalized_category = utils::substring(utils::to_lower_copy(category), 0, TAG_LENGTH_MAX);

        auto rng = idx.equal_range(boost::make_tuple(normalized_domain, normalized_category));

        std::vector<std::pair<std::string, uint32_t>> ret;
        boost::transform(rng, std::back_inserter(ret),
                         [](const tags::category_stats_object& o) { return std::make_pair(o.tag, o.tags_count); });
        return ret;
    }

    std::vector<discussion> get_discussions_by_trending(const discussion_query& query) const
    {
        auto ordering = [](const tag_object& lhs, const tag_object& rhs) { return lhs.trending > rhs.trending; };
        auto filter = [](const tag_object& t) { return t.net_rshares > 0; };

        return get_discussions(query, ordering, filter);
    }

    std::vector<discussion> get_discussions_by_created(const discussion_query& query) const
    {
        auto ordering = [](const tag_object& lhs, const tag_object& rhs) {
            return std::tie(lhs.created, lhs.comment) > std::tie(rhs.created, rhs.comment);
        };

        return get_discussions(query, ordering);
    }

    std::vector<discussion> get_discussions_by_hot(const discussion_query& query) const
    {
        auto ordering = [](const tag_object& lhs, const tag_object& rhs) { return lhs.hot > rhs.hot; };
        auto filter = [](const tag_object& t) { return t.net_rshares > 0; };

        return get_discussions(query, ordering, filter);
    }

    std::vector<discussion> get_discussions_by_author(const discussion_query& query) const
    {
        FC_ASSERT(query.limit <= MAX_DISCUSSIONS_LIST_SIZE,
                  "limit cannot be more than " + std::to_string(MAX_DISCUSSIONS_LIST_SIZE));
        FC_ASSERT(query.start_author && !query.start_author->empty(),
                  "start_author should be specified and cannot be empty");

        return get_discussions_by_author(*query.start_author, query.start_permlink, query.limit);
    }

    discussion get_content(const std::string& author, const std::string& permlink) const
    {
        const auto& by_permlink_idx = _db.get_index<comment_index, by_permlink>();
        auto itr = by_permlink_idx.find(boost::make_tuple(author, permlink));
        if (itr != by_permlink_idx.end())
        {
            return get_discussion(*itr);
        }
        return discussion();
    }

    std::vector<discussion>
    get_comments(const std::string& parent_author, const std::string& parent_permlink, uint32_t depth) const
    {
        FC_ASSERT(!parent_author.empty(), "parent_author could't be empty.");
        FC_ASSERT(!parent_permlink.empty(), "parent_permlink could't be empty.");

        std::vector<discussion> result;

        comments_traverse traverse(_services.comment_service());

        traverse.find_children(parent_author, parent_permlink, [&](const comment_object& comment) {
            if (comment.depth <= depth)
            {
                result.push_back(get_discussion(comment));
            }
        });

        return result;
    }

private:
    scorum::chain::database& _db;
    scorum::chain::data_service_factory_i& _services;
    tags_service _tags_service;

    static bool tag_filter_default(const tags::tag_object&)
    {
        return true;
    }

    void set_url(discussion& d) const
    {
        const api::comment_api_obj root(_services.comment_service().get(d.root_comment));
        d.url = "/" + root.category + "/@" + root.author + "/" + root.permlink;
        d.root_title = root.title;
        if (root.id != d.id)
            d.url += "#@" + d.author + "/" + d.permlink;
    }

    discussion get_discussion(comment_id_type id, uint32_t truncate_body = 0) const
    {
        return get_discussion(_services.comment_service().get(id), truncate_body);
    }

    discussion get_discussion(const comment_object& comment, uint32_t truncate_body = 0) const
    {
        discussion d = create_discussion(comment);

        set_url(d);
        set_pending_payout(d);

        d.active_votes = get_active_votes(d.author, d.permlink);
        d.body_length = d.body.size();

        if (truncate_body)
        {
            d.body = d.body.substr(0, truncate_body);

            if (!fc::is_utf8(d.body))
                d.body = fc::prune_invalid_utf8(d.body);
        }

        return d;
    }

    u256 to256(const fc::uint128& t) const
    {
        u256 result(t.high_bits());
        result <<= 65;
        result += t.low_bits();
        return result;
    }

    void set_pending_payout(discussion& d) const
    {
        _tags_service.set_promoted_balance(d.id, d.promoted);

        d.pending_payout_scr = calc_pending_payout(d, _services.content_reward_fund_scr_service().get());

        d.pending_payout_sp = calc_pending_payout(d, _services.content_reward_fund_sp_service().get());

        if (d.parent_author != SCORUM_ROOT_POST_PARENT_ACCOUNT)
            d.cashout_time = _tags_service.calculate_discussion_payout_time(_services.comment_service().get(d.id));

        if (d.body.size() > 1024 * 128)
            d.body = "body pruned due to size";

        if (d.parent_author.size() > 0 && d.body.size() > 1024 * 65)
            d.body = "comment pruned due to size";

        set_url(d);
    }

    std::vector<api::vote_state> get_active_votes(const std::string& author, const std::string& permlink) const
    {
        std::vector<api::vote_state> result;
        const auto& comment = _services.comment_service().get(author, permlink);

        auto votes = _services.comment_vote_service().get_by_comment(comment.id);

        for (auto it = votes.begin(); it != votes.end(); ++it)
        {
            const auto comment_voute = it->get();
            const auto& vouter = _services.account_service().get(comment_voute.voter);

            api::vote_state vstate;
            vstate.voter = vouter.name;
            vstate.weight = comment_voute.weight;
            vstate.rshares = comment_voute.rshares.value;
            vstate.percent = comment_voute.vote_percent;
            vstate.time = comment_voute.last_update;

            result.push_back(vstate);
        }

        return result;
    }

    posts_crefs get_posts(const std::string& tag,
                          const std::function<bool(const tag_object&)>& tag_filter = &tag_filter_default) const
    {
        std::string tag_lower = utils::to_lower_copy(tag);

        const auto& tag_idx = _db.get_index<tags::tag_index, tags::by_tag>();

        auto rng = tag_idx.equal_range(tag_lower) | boost::adaptors::filtered(tag_filter);

        posts_crefs posts;
        boost::copy(rng, std::back_inserter(posts));

        return posts;
    }

    posts_crefs intersect(const std::vector<posts_crefs>& posts_by_tags) const
    {
        using post_cref = posts_crefs::value_type;

        auto from = posts_by_tags.begin();
        auto to = posts_by_tags.end();

        posts_crefs start(from->begin(), from->end());
        auto tail = boost::make_iterator_range(std::next(from), to);

        /// Each collection in 'posts_by_tags' is already sorted by comment id (see 'tag_index/by_comment').
        /// Intersect collections in 'posts_by_tags' each with other.
        /// [[a,b,c],[b,c,d],[c,d,e]] => start: [a,b,c]; tail: [[b,c,d], [c,d,e]]
        /// 1. [a,b,c] /\ [b,c,d] = [b,c] (i.e. 'intersection' = [b,c])
        /// 2. [b,c] /\ [c,d,e] = [c] (i.e 'intersection' = [c])
        // clang-format off
        posts_crefs intersection = boost::accumulate(tail, start, [](posts_crefs& intersection, const posts_crefs& posts) {
            /// intersect 'intersection' with 'posts' and put result back into 'intersection'
            auto upper_bound = boost::set_intersection(intersection, posts, intersection.begin(), [](post_cref lhs, post_cref rhs) {
                return lhs.get().comment < rhs.get().comment;
            });
            intersection.erase(upper_bound, intersection.end());

            return intersection;
        });
        // clang-format on

        return intersection;
    }

    posts_crefs union_all(const std::vector<posts_crefs>& posts_by_tags) const
    {
        using post_cref = posts_crefs::value_type;

        struct less_by_comment
        {
            bool operator()(post_cref lhs, post_cref rhs) const
            {
                return lhs.get().comment < rhs.get().comment;
            }
        };

        std::set<post_cref, less_by_comment> set;
        for (const auto& posts : posts_by_tags)
            boost::copy(posts, std::inserter(set, end(set)));

        posts_crefs result;
        result.reserve(set.size());
        result.assign(set.begin(), set.end());

        return result;
    }

    std::vector<discussion> get_discussions(const discussion_query& query,
                                            const std::function<bool(const tag_object&, const tag_object&)>& ordering,
                                            const std::function<bool(const tag_object&)>& tag_filter
                                            = &tag_filter_default) const
    {
        // clang-format off
        FC_ASSERT(query.limit <= MAX_DISCUSSIONS_LIST_SIZE,
                  "limit cannot be more than " + std::to_string(MAX_DISCUSSIONS_LIST_SIZE));
        FC_ASSERT((query.start_author && query.start_permlink && !query.start_author->empty() && !query.start_permlink->empty()) ||
                  (!query.start_author && !query.start_permlink),
                  "start_author and start_permlink should be either both specified and not empty or both not specified");

        auto rng = query.tags
            | boost::adaptors::transformed(utils::to_lower_copy)
            | boost::adaptors::transformed([](const std::string& s) { return utils::substring(s, 0, TAG_LENGTH_MAX); });
        // clang-format on

        std::set<std::string> tags(rng.begin(), rng.end());
        if (tags.empty())
            tags.insert("");

        std::vector<posts_crefs> posts_by_tags;
        posts_by_tags.reserve(tags.size());

        boost::transform(tags, std::back_inserter(posts_by_tags),
                         [&](const std::string& t) { return get_posts(t, tag_filter); });

        // clang-format off
        posts_crefs posts = query.tags_logical_and
                ? intersect(posts_by_tags)
                : union_all(posts_by_tags);
        // clang-format on

        std::vector<discussion> result;
        if (posts.empty())
            return result;

        boost::sort(posts, ordering);

        posts_crefs::value_type threshold = *(posts.begin());
        if (query.start_author && query.start_permlink)
        {
            auto id = _services.comment_service().get(*query.start_author, *query.start_permlink).id;
            const auto& comment_idx = _db.get_index<tags::tag_index, tags::by_comment>();
            threshold = *(comment_idx.find(id));
        }

        auto it = boost::lower_bound(posts, threshold.get(), ordering);

        for (; it != posts.end() && result.size() < query.limit; ++it)
        {
            try
            {
                result.push_back(get_discussion(it->get().comment, query.truncate_body));
                result.back().promoted = asset(it->get().promoted_balance, SCORUM_SYMBOL);
            }
            catch (const fc::exception& e)
            {
                edump((e.to_detail_string()));
            }
        }

        return result;
    }

    std::vector<discussion>
    get_discussions_by_author(const std::string& author, fc::optional<std::string> start_permlink, uint32_t limit) const
    {
        std::vector<discussion> result;

#ifndef IS_LOW_MEM

        result.reserve(limit);

        const auto& idx = _db.get_index<comment_index, by_author_created>();

        auto it = idx.lower_bound(author);
        if (start_permlink && !start_permlink->empty())
            it = idx.iterator_to(_services.comment_service().get(author, *start_permlink));

        while (it != idx.end() && it->author == author && result.size() < limit)
        {
            if (it->parent_author.size() == 0)
            {
                result.push_back(get_discussion(*it));
            }
            ++it;
        }
#endif
        return result;
    }

    class comments_traverse
    {
    public:
        comments_traverse(scorum::chain::comment_service_i& s)
            : _comment_service(s)
        {
        }

        template <typename OnItem>
        void find_children(const std::string& parent_author, const std::string& parent_permlink, OnItem&& on_item)
        {
            account_name_type account_name = account_name_type(parent_author);

            scan_children(account_name, parent_permlink);

            while (!_stack.empty())
            {
                auto comment = _stack.top();
                _stack.pop();

                on_item(comment.get());

                scan_children(comment.get().author, comment.get().permlink);
            }
        }

    private:
        void scan_children(const account_name_type& parent_author, const fc::shared_string& parent_permlink)
        {
            scan_children(parent_author, fc::to_string(parent_permlink));
        }

        void scan_children(const account_name_type& parent_author, const std::string& parent_permlink)
        {
            auto range = _comment_service.get_children(parent_author, parent_permlink);

            for (auto itr = range.rbegin(); itr != range.rend(); ++itr)
            {
                _stack.push(*itr);
            }
        }

        std::stack<scorum::chain::comment_service_i::object_cref_type> _stack;

        scorum::chain::comment_service_i& _comment_service;
    };
};

} // namespace tags
} // namespace scorum
