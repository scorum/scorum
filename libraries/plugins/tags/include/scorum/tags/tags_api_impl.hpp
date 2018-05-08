#pragma once

#include <boost/algorithm/string.hpp>

#include <stack>

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

    std::vector<discussion>
    get_comments(const std::string& parent_author, const std::string& parent_permlink, uint32_t depth) const
    {
        FC_ASSERT(!parent_author.empty(), "parent_author could't be empty.");
        FC_ASSERT(!parent_permlink.empty(), "parent_permlink could't be empty.");

        const auto& index = _db.get_index<comment_index>().indices().get<by_parent>();
        std::vector<discussion> result;

        index_traverse traverse(index);

        traverse.find_comments(parent_author, parent_permlink, [&](const comment_object& comment) {
            if (comment.depth <= depth)
            {
                result.push_back(discussion(comment));
                set_pending_payout(result.back());
            }
        });

        return result;
    }

    std::vector<discussion> get_replies_by_last_update(account_name_type start_parent_author,
                                                       const std::string& start_permlink,
                                                       uint32_t limit) const
    {
        std::vector<discussion> result;

#ifndef IS_LOW_MEM
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
#endif

        return result;
    }

    std::vector<discussion> get_discussions_by_author_before_date(const std::string& author,
                                                                  const std::string& start_permlink,
                                                                  time_point_sec before_date,
                                                                  uint32_t limit) const
    {

        std::vector<discussion> result;

#ifndef IS_LOW_MEM
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
#endif
        return result;
    }

    api::state get_state(std::string path) const
    {
        state _state;
        _state.props = _services.dynamic_global_property_service().get();
        _state.current_route = path;

        try
        {
            if (path.size() && path[0] == '/')
                path = path.substr(1); /// remove '/' from front

            if (!path.size())
                path = "trending";

            /// FETCH CATEGORY STATE
            auto trending_tags = get_trending_tags(std::string(), 50);
            for (const auto& t : trending_tags)
            {
                _state.tag_idx.trending.push_back(std::string(t.name));
            }
            /// END FETCH CATEGORY STATE

            std::set<std::string> accounts;

            std::vector<std::string> part;
            part.reserve(4);
            boost::split(part, path, boost::is_any_of("/"));
            part.resize(std::max(part.size(), size_t(4))); // at least 4

            auto tag = fc::to_lower(part[1]);

            if (part[0].size() && part[0][0] == '@')
            {
                auto acnt = part[0].substr(1);
                _state.accounts[acnt]
                    = extended_account(_db.obtain_service<chain::dbs_account>().get_account(acnt), _db);
                _state.accounts[acnt].tags_usage = get_tags_used_by_author(acnt);

                auto& eacnt = _state.accounts[acnt];
                if (part[1] == "transfers")
                {
                    // TODO: rework this garbage method - split it into sensible parts
                    // auto history = get_account_history(acnt, uint64_t(-1), 10000);
                    // for (auto& item : history)
                    //{
                    //    switch (item.second.op.which())
                    //    {
                    //    case operation::tag<transfer_to_scorumpower_operation>::value:
                    //    case operation::tag<withdraw_scorumpower_operation>::value:
                    //    case operation::tag<transfer_operation>::value:
                    //    case operation::tag<author_reward_operation>::value:
                    //    case operation::tag<curation_reward_operation>::value:
                    //    case operation::tag<comment_benefactor_reward_operation>::value:
                    //    case operation::tag<escrow_transfer_operation>::value:
                    //    case operation::tag<escrow_approve_operation>::value:
                    //    case operation::tag<escrow_dispute_operation>::value:
                    //    case operation::tag<escrow_release_operation>::value:
                    //        eacnt.transfer_history[item.first] = item.second;
                    //        break;
                    //    case operation::tag<comment_operation>::value:
                    //        //   eacnt.post_history[item.first] =  item.second;
                    //        break;
                    //    case operation::tag<vote_operation>::value:
                    //    case operation::tag<account_witness_vote_operation>::value:
                    //    case operation::tag<account_witness_proxy_operation>::value:
                    //        //   eacnt.vote_history[item.first] =  item.second;
                    //        break;
                    //    case operation::tag<account_create_operation>::value:
                    //    case operation::tag<account_update_operation>::value:
                    //    case operation::tag<witness_update_operation>::value:
                    //    case operation::tag<producer_reward_operation>::value:
                    //    default:
                    //        eacnt.other_history[item.first] = item.second;
                    //    }
                    //}
                }
                else if (part[1] == "recent-replies")
                {
                    auto replies = get_replies_by_last_update(acnt, "", 50);
                    eacnt.recent_replies = std::vector<std::string>();
                    for (const auto& reply : replies)
                    {
                        auto reply_ref = reply.author + "/" + reply.permlink;
                        _state.content[reply_ref] = reply;

                        eacnt.recent_replies->push_back(reply_ref);
                    }
                }
                else if (part[1] == "posts" || part[1] == "comments")
                {
#ifndef IS_LOW_MEM
                    int count = 0;
                    const auto& pidx = _db.get_index<comment_index>().indices().get<by_author_last_update>();
                    auto itr = pidx.lower_bound(acnt);
                    eacnt.comments = std::vector<std::string>();

                    while (itr != pidx.end() && itr->author == acnt && count < 20)
                    {
                        if (itr->parent_author.size())
                        {
                            const auto link = acnt + "/" + fc::to_string(itr->permlink);
                            eacnt.comments->push_back(link);
                            _state.content[link] = *itr;
                            set_pending_payout(_state.content[link]);
                            ++count;
                        }

                        ++itr;
                    }
#endif
                }
            }
            /// pull a complete discussion
            else if (part[1].size() && part[1][0] == '@')
            {
                auto account = part[1].substr(1);
                auto slug = part[2];

                auto key = account + "/" + slug;
                auto dis = get_content(account, slug);

                recursively_fetch_content(_state, dis, accounts);
                _state.content[key] = std::move(dis);
            }
            else if (part[0] == "witnesses" || part[0] == "~witnesses")
            {
                auto wits = get_witnesses_by_vote("", 50);
                for (const auto& w : wits)
                {
                    _state.witnesses[w.owner] = w;
                }
            }
            else if (part[0] == "trending")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_trending(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.trending.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "payout")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_post_discussions_by_payout(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.payout.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "payout_comments")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_comment_discussions_by_payout(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.payout_comments.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "promoted")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_promoted(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.promoted.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "responses")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_children(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.responses.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (!part[0].size() || part[0] == "hot")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_hot(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.hot.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (!part[0].size() || part[0] == "promoted")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_promoted(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.promoted.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "votes")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_votes(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.votes.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "cashout")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_cashout(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.cashout.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "active")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_active(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.active.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "created")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_created(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.created.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "recent")
            {
                discussion_query q;
                q.tag = tag;
                q.limit = 20;
                q.truncate_body = 1024;
                auto trending_disc = get_discussions_by_created(q);

                auto& didx = _state.discussion_idx[tag];
                for (const auto& d : trending_disc)
                {
                    auto key = d.author + "/" + d.permlink;
                    didx.created.push_back(key);
                    if (d.author.size())
                        accounts.insert(d.author);
                    _state.content[key] = std::move(d);
                }
            }
            else if (part[0] == "tags")
            {
                _state.tag_idx.trending.clear();
                auto trending_tags = get_trending_tags(std::string(), 250);
                for (const auto& t : trending_tags)
                {
                    std::string name = t.name;
                    _state.tag_idx.trending.push_back(name);
                    _state.tags[name] = t;
                }
            }
            else
            {
                elog("What... no matches");
            }

            chain::dbs_account& account_service = _db.obtain_service<chain::dbs_account>();
            for (const auto& a : accounts)
            {
                _state.accounts.erase("");
                _state.accounts[a] = extended_account(account_service.get_account(a), _db);
            }
            for (auto& d : _state.content)
            {
                d.second.active_votes = get_active_votes(d.second.author, d.second.permlink);
            }

            _state.witness_schedule = _db.obtain_service<dbs_witness_schedule>().get();
        }
        catch (const fc::exception& e)
        {
            _state.error = e.to_detail_string();
        }
        return _state;
    }

private:
    scorum::chain::database& _db;
    scorum::chain::data_service_factory_i& _services;
    tags_service _tags_service;

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

        if (d.cashout_time != fc::time_point_sec::maximum())
        {
            {
                const auto& reward_fund_obj = _services.content_reward_fund_scr_service().get();
                share_type pending_payout_value;
                if (reward_fund_obj.recent_claims > 0)
                {
                    pending_payout_value = rewards_math::predict_payout(
                        reward_fund_obj.recent_claims, reward_fund_obj.activity_reward_balance.amount, d.net_rshares,
                        reward_fund_obj.author_reward_curve, d.max_accepted_payout.amount,
                        SCORUM_RECENT_RSHARES_DECAY_RATE, SCORUM_MIN_COMMENT_PAYOUT_SHARE);
                }
                d.pending_payout_scr_value = asset(pending_payout_value, SCORUM_SYMBOL);
            }

            {
                const auto& reward_fund_obj = _services.content_reward_fund_sp_service().get();
                share_type pending_payout_value;
                if (reward_fund_obj.recent_claims > 0)
                {
                    pending_payout_value = rewards_math::predict_payout(
                        reward_fund_obj.recent_claims, reward_fund_obj.activity_reward_balance.amount, d.net_rshares,
                        reward_fund_obj.author_reward_curve, d.max_accepted_payout.amount,
                        SCORUM_RECENT_RSHARES_DECAY_RATE, SCORUM_MIN_COMMENT_PAYOUT_SHARE);
                }
                d.pending_payout_sp_value = asset(pending_payout_value, SP_SYMBOL);
            }
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
            vstate.rshares = comment_voute.rshares.value;
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

    /**
     *  This call assumes root already stored as part of state, it will
     *  modify root.replies to contain links to the reply posts and then
     *  add the reply discussions to the state. This method also fetches
     *  any accounts referenced by authors.
     *
     */
    void recursively_fetch_content(state& _state, discussion& root, std::set<std::string>& referenced_accounts) const
    {
        try
        {
            if (root.author.size())
                referenced_accounts.insert(root.author);

            auto replies = get_content_replies(root.author, root.permlink);
            for (auto& r : replies)
            {
                try
                {
                    recursively_fetch_content(_state, r, referenced_accounts);
                    root.replies.push_back(r.author + "/" + r.permlink);
                    _state.content[r.author + "/" + r.permlink] = std::move(r);
                    if (r.author.size())
                        referenced_accounts.insert(r.author);
                }
                catch (const fc::exception& e)
                {
                    edump((e.to_detail_string()));
                }
            }
        }
        FC_CAPTURE_AND_RETHROW((root.author)(root.permlink))
    }

    std::vector<witness_api_obj> get_witnesses_by_vote(const std::string& from, uint32_t limit) const
    {
        FC_ASSERT(limit <= 100);

        std::vector<witness_api_obj> result;
        result.reserve(limit);

        const auto& name_idx = _db.get_index<witness_index>().indices().get<by_name>();
        const auto& vote_idx = _db.get_index<witness_index>().indices().get<by_vote_name>();

        auto itr = vote_idx.begin();
        if (from.size())
        {
            auto nameitr = name_idx.find(from);
            FC_ASSERT(nameitr != name_idx.end(), "invalid witness name ${n}", ("n", from));
            itr = vote_idx.iterator_to(*nameitr);
        }

        while (itr != vote_idx.end() && result.size() < limit && itr->votes > 0)
        {
            result.push_back(witness_api_obj(*itr));
            ++itr;
        }
        return result;
    }

    class index_traverse
    {
        typedef comment_index::index_iterator<by_parent>::type search_iterator;

    public:
        typedef comment_index::index<by_parent>::type Index;

        index_traverse(const Index& index)
            : _index(index)
        {
        }

        template <typename OnItem>
        void find_comments(const std::string& parent_author, const std::string& parent_permlink, OnItem&& on_item)
        {
            account_name_type account_name = account_name_type(parent_author);

            scan_children(account_name, parent_permlink);

            while (!_stack.empty())
            {
                search_iterator itr = _stack.top();
                _stack.pop();

                on_item(*itr);

                scan_children(itr->author, fc::to_string(itr->permlink));
            }
        }

    private:
        const Index& _index;

        void scan_children(const account_name_type& parent_author, const std::string& parent_permlink)
        {
            auto itr = _index.find(boost::make_tuple(parent_author, parent_permlink));

            if (itr != _index.end())
            {
                put_in_stack(itr);
            }
        }

        template <typename StartItr> void put_in_stack(StartItr& itr)
        {
            auto parent_author = itr->parent_author;
            auto parent_permlink = itr->parent_permlink;

            std::vector<search_iterator> array;

            while (itr != _index.end() && itr->parent_author == parent_author
                   && itr->parent_permlink == parent_permlink)
            {
                array.push_back(itr);
                ++itr;
            }

            for (auto itr = array.rbegin(); itr != array.rend(); ++itr)
            {
                _stack.push(*itr);
            }
        }

        std::stack<search_iterator> _stack;
    };
};

} // namespace tags
} // namespace scorum
