#pragma once

#include <scorum/protocol/types.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/util/reward.hpp>

#include <scorum/common_api/config.hpp>
#include <scorum/tags/tags_api_objects.hpp>
#include <scorum/tags/tags_service.hpp>

namespace scorum {
namespace tags {

using namespace scorum::tags::api;

class tags_api_impl
{
public:
    tags_api_impl(scorum::chain::database& db)
        : _db(db)
        , _services(_db)
        , _tags_service(_db)
    {
    }

    ~tags_api_impl()
    {
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
        const auto& acnt = _db.obtain_service<dbs_account>().get_account(author);

        const auto& tidx = _db.get_index<tags::author_tag_stats_index>().indices().get<tags::by_author_posts_tag>();
        auto itr = tidx.lower_bound(boost::make_tuple(acnt.id, 0));
        std::vector<std::pair<std::string, uint32_t>> result;

        while (itr != tidx.end() && itr->author == acnt.id && result.size() < LOOKUP_LIMIT)
        {
            result.push_back(std::make_pair(itr->tag, itr->total_posts));
            ++itr;
        }

        return result;
    }

    std::vector<discussion> get_discussions_by_payout(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = get_parent(query);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_net_rshares>();
        auto tidx_itr = tidx.lower_bound(tag);

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body,
                               [](const comment_api_obj& c) { return c.net_rshares <= 0; }, exit_default,
                               tag_exit_default, true);
    }

    std::vector<discussion> get_post_discussions_by_payout(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = comment_id_type();

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_reward_fund_net_rshares>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, true));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body,
                               [](const comment_api_obj& c) { return c.net_rshares <= 0; }, exit_default,
                               tag_exit_default, true);
    }

    std::vector<discussion> get_comment_discussions_by_payout(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = comment_id_type(1);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_reward_fund_net_rshares>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, false));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body,
                               [](const comment_api_obj& c) { return c.net_rshares <= 0; }, exit_default,
                               tag_exit_default, true);
    }

    std::vector<discussion> get_discussions_by_trending(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = get_parent(query);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_parent_trending>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, parent, std::numeric_limits<double>::max()));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body,
                               [](const comment_api_obj& c) { return c.net_rshares <= 0; });
    }

    std::vector<discussion> get_discussions_by_created(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = get_parent(query);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_parent_created>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, parent, fc::time_point_sec::maximum()));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body);
    }

    std::vector<discussion> get_discussions_by_hot(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = get_parent(query);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_parent_hot>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, parent, std::numeric_limits<double>::max()));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body,
                               [](const comment_api_obj& c) { return c.net_rshares <= 0; });
    }

    std::vector<discussion> get_discussions_by_promoted(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = get_parent(query);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_parent_promoted>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, parent, share_type(SCORUM_MAX_SHARE_SUPPLY)));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body, filter_default, exit_default,
                               [](const tags::tag_object& t) { return t.promoted_balance == 0; });
    }

    std::vector<discussion> get_discussions_by_active(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = get_parent(query);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_parent_active>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, parent, fc::time_point_sec::maximum()));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body);
    }

    std::vector<discussion> get_discussions_by_cashout(const discussion_query& query) const
    {
        query.validate();
        std::vector<discussion> result;

        auto tag = fc::to_lower(query.tag);
        auto parent = get_parent(query);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_cashout>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, fc::time_point::now() - fc::minutes(60)));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body,
                               [](const comment_api_obj& c) { return c.net_rshares < 0; });
    }

    std::vector<discussion> get_discussions_by_votes(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = get_parent(query);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_parent_net_votes>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, parent, std::numeric_limits<int32_t>::max()));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body);
    }

    std::vector<discussion> get_discussions_by_children(const discussion_query& query) const
    {
        query.validate();
        auto tag = fc::to_lower(query.tag);
        auto parent = get_parent(query);

        const auto& tidx = _db.get_index<tags::tag_index>().indices().get<tags::by_parent_children>();
        auto tidx_itr = tidx.lower_bound(boost::make_tuple(tag, parent, std::numeric_limits<int32_t>::max()));

        return get_discussions(query, tag, parent, tidx, tidx_itr, query.truncate_body);
    }

    std::vector<discussion> get_discussions_by_comments(const discussion_query& query) const
    {
        std::vector<discussion> result;
#ifndef IS_LOW_MEM
        query.validate();
        FC_ASSERT(query.start_author, "Must get comments for a specific author");
        auto start_author = *(query.start_author);
        auto start_permlink = query.start_permlink ? *(query.start_permlink) : "";

        const auto& c_idx = _db.get_index<comment_index>().indices().get<by_permlink>();
        const auto& t_idx = _db.get_index<comment_index>().indices().get<by_author_last_update>();
        auto comment_itr = t_idx.lower_bound(start_author);

        if (start_permlink.size())
        {
            auto start_c = c_idx.find(boost::make_tuple(start_author, start_permlink));
            FC_ASSERT(start_c != c_idx.end(), "Comment is not in account's comments");
            comment_itr = t_idx.iterator_to(*start_c);
        }

        result.reserve(query.limit);

        while (result.size() < query.limit && comment_itr != t_idx.end())
        {
            if (comment_itr->author != start_author)
                break;
            if (comment_itr->parent_author.size() > 0)
            {
                try
                {
                    result.push_back(get_discussion(comment_itr->id));
                }
                catch (const fc::exception& e)
                {
                    edump((e.to_detail_string()));
                }
            }

            ++comment_itr;
        }
#endif
        return result;
    }

    discussion get_content(const std::string& author, const std::string& permlink) const
    {
        const auto& by_permlink_idx = _db.get_index<comment_index>().indices().get<by_permlink>();
        auto itr = by_permlink_idx.find(boost::make_tuple(author, permlink));
        if (itr != by_permlink_idx.end())
        {
            discussion result(*itr);
            set_pending_payout(result);
            result.active_votes = get_active_votes(author, permlink);
            return result;
        }
        return discussion();
    }

    std::vector<discussion> get_content_replies(const std::string& parent, const std::string& parent_permlink) const
    {
        account_name_type acc_name = account_name_type(parent);
        const auto& by_permlink_idx = _db.get_index<comment_index>().indices().get<by_parent>();
        auto itr = by_permlink_idx.find(boost::make_tuple(acc_name, parent_permlink));
        std::vector<discussion> result;
        while (itr != by_permlink_idx.end() && itr->parent_author == parent
               && fc::to_string(itr->parent_permlink) == parent_permlink)
        {
            result.push_back(discussion(*itr));
            set_pending_payout(result.back());
            ++itr;
        }
        return result;
    }

    std::vector<discussion> get_replies_by_last_update(account_name_type start_parent_author,
                                                       const std::string& start_permlink,
                                                       uint32_t limit) const
    {
        std::vector<discussion> result;

#ifdef IS_LOW_MEM
        return result;
#endif

        FC_ASSERT(limit <= 100);
        const auto& last_update_idx = _db.get_index<comment_index>().indices().get<by_last_update>();
        auto itr = last_update_idx.begin();
        const account_name_type* parent_author = &start_parent_author;

        if (start_permlink.size())
        {
            const auto& comment = _db.obtain_service<dbs_comment>().get(start_parent_author, start_permlink);
            itr = last_update_idx.iterator_to(comment);
            parent_author = &comment.parent_author;
        }
        else if (start_parent_author.size())
        {
            itr = last_update_idx.lower_bound(start_parent_author);
        }

        result.reserve(limit);

        while (itr != last_update_idx.end() && result.size() < limit && itr->parent_author == *parent_author)
        {
            result.push_back(*itr);
            set_pending_payout(result.back());
            result.back().active_votes = get_active_votes(itr->author, fc::to_string(itr->permlink));
            ++itr;
        }

        return result;
    }

    std::vector<discussion> get_discussions_by_author_before_date(const std::string& author,
                                                                  const std::string& start_permlink,
                                                                  time_point_sec before_date,
                                                                  uint32_t limit) const
    {

        std::vector<discussion> result;

#ifdef IS_LOW_MEM
        return result;
#endif

        FC_ASSERT(limit <= 100);
        result.reserve(limit);
        uint32_t count = 0;
        const auto& didx = _db.get_index<comment_index>().indices().get<by_author_last_update>();

        if (before_date == time_point_sec())
            before_date = time_point_sec::maximum();

        auto itr = didx.lower_bound(boost::make_tuple(author, time_point_sec::maximum()));
        if (start_permlink.size())
        {
            const auto& comment = _db.obtain_service<dbs_comment>().get(author, start_permlink);
            if (comment.created < before_date)
                itr = didx.iterator_to(comment);
        }

        while (itr != didx.end() && itr->author == author && count < limit)
        {
            if (itr->parent_author.size() == 0)
            {
                result.push_back(*itr);
                set_pending_payout(result.back());
                result.back().active_votes = get_active_votes(itr->author, fc::to_string(itr->permlink));
                ++count;
            }
            ++itr;
        }
        return result;
    }

private:
    static bool filter_default(const comment_api_obj&)
    {
        return false;
    }

    static bool exit_default(const comment_api_obj&)
    {
        return false;
    }

    static bool tag_exit_default(const tags::tag_object&)
    {
        return false;
    }

    comment_id_type get_parent(const discussion_query& query) const
    {
        comment_id_type parent;

        if (query.parent_author && query.parent_permlink)
        {
            parent = get_parent(*query.parent_author, *query.parent_permlink);
        }

        return parent;
    }

    comment_id_type get_parent(const std::string& parent_author, const std::string& parent_permlink) const
    {
        return _services.comment_service().get(parent_author, parent_permlink).id;
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
        discussion d = _services.comment_service().get(id);

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

        const auto& reward_fund_obj = _services.reward_fund_service().get();

        asset pot = reward_fund_obj.activity_reward_balance_scr;
        u256 total_r2 = to256(reward_fund_obj.recent_claims);
        if (total_r2 > 0)
        {
            uint128_t vshares;
            vshares = d.net_rshares.value > 0
                ? scorum::chain::util::evaluate_reward_curve(d.net_rshares.value, reward_fund_obj.author_reward_curve)
                : 0;

            u256 r2 = to256(vshares); // to256(abs_net_rshares);
            r2 *= pot.amount.value;
            r2 /= total_r2;

            d.pending_payout_value = asset(static_cast<uint64_t>(r2), pot.symbol());
        }

        if (d.parent_author != SCORUM_ROOT_POST_PARENT_ACCOUNT)
            d.cashout_time = _tags_service.calculate_discussion_payout_time(_services.comment_service().get(d.id));

        if (d.body.size() > 1024 * 128)
            d.body = "body pruned due to size";
        if (d.parent_author.size() > 0 && d.body.size() > 1024 * 16)
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
            vstate.rshares = comment_voute.rshares;
            vstate.percent = comment_voute.vote_percent;
            vstate.time = comment_voute.last_update;

            result.push_back(vstate);
        }

        return result;
    }

    template <typename Index, typename StartItr>
    std::vector<discussion> get_discussions(const discussion_query& query,
                                            const std::string& tag,
                                            comment_id_type parent,
                                            const Index& tidx,
                                            StartItr tidx_itr,
                                            uint32_t truncate_body = 0,
                                            const std::function<bool(const comment_api_obj&)>& filter = &filter_default,
                                            const std::function<bool(const comment_api_obj&)>& exit = &exit_default,
                                            const std::function<bool(const tag_object&)>& tag_exit = &tag_exit_default,
                                            bool ignore_parent = false) const
    {
        std::vector<discussion> result;

        const auto& cidx = _db.get_index<tags::tag_index>().indices().get<tags::by_comment>();
        comment_id_type start;

        if (query.start_author && query.start_permlink)
        {
            start = _db.obtain_service<dbs_comment>().get(*query.start_author, *query.start_permlink).id;
            auto itr = cidx.find(start);
            while (itr != cidx.end() && itr->comment == start)
            {
                if (itr->tag == tag)
                {
                    tidx_itr = tidx.iterator_to(*itr);
                    break;
                }
                ++itr;
            }
        }

        uint32_t count = query.limit;
        uint64_t itr_count = 0;
        uint64_t filter_count = 0;
        uint64_t exc_count = 0;
        uint64_t max_itr_count = 10 * query.limit;

        while (count > 0 && tidx_itr != tidx.end())
        {
            ++itr_count;
            if (itr_count > max_itr_count)
            {
                wlog("Maximum iteration count exceeded serving query: ${q}", ("q", query));
                wlog("count=${count}   itr_count=${itr_count}   filter_count=${filter_count}   exc_count=${exc_count}",
                     ("count", count)("itr_count", itr_count)("filter_count", filter_count)("exc_count", exc_count));
                break;
            }
            if (tidx_itr->tag != tag || (!ignore_parent && tidx_itr->parent != parent))
                break;
            try
            {
                result.push_back(get_discussion(tidx_itr->comment, truncate_body));
                result.back().promoted = asset(tidx_itr->promoted_balance, SCORUM_SYMBOL);

                if (filter(result.back()))
                {
                    result.pop_back();
                    ++filter_count;
                }
                else if (exit(result.back()) || tag_exit(*tidx_itr))
                {
                    result.pop_back();
                    break;
                }
                else
                    --count;
            }
            catch (const fc::exception& e)
            {
                ++exc_count;
                edump((e.to_detail_string()));
            }
            ++tidx_itr;
        }
        return result;
    }

    scorum::chain::database& _db;
    scorum::chain::data_service_factory_i& _services;
    tags_service _tags_service;
};

} // namespace tags
} // namespace scorum
