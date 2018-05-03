#pragma once

#include <scorum/protocol/types.hpp>

#include <scorum/chain/services/comment_statistic.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

#include <scorum/app/scorum_api_objects.hpp>
#include <scorum/app/state.hpp>

#include <scorum/tags/tags_objects.hpp>

namespace scorum {
namespace tags {
namespace api {

using fc::time_point_sec;

using scorum::protocol::asset;
using scorum::protocol::account_name_type;
using scorum::protocol::share_type;

using scorum::chain::comment_id_type;
using scorum::chain::beneficiary_route_type;
using scorum::chain::comment_object;
using scorum::chain::account_object;

using scorum::app::dynamic_global_property_api_obj;
using scorum::app::witness_schedule_api_obj;
using scorum::app::extended_account;
using scorum::app::vote_state;
using scorum::app::account_vote;
using scorum::app::witness_api_obj;
using scorum::app::account_api_obj;

struct comment_api_obj
{
    comment_api_obj()
    {
    }

    comment_api_obj(const scorum::chain::comment_object& o);

    comment_api_obj(const chain::comment_object& o,
                    const comment_statistic_scr_service_i&,
                    const comment_statistic_sp_service_i&);

    comment_id_type id;
    std::string category;
    account_name_type parent_author;
    std::string parent_permlink;
    account_name_type author;
    std::string permlink;

    std::string title;
    std::string body;
    std::string json_metadata;
    time_point_sec last_update;
    time_point_sec created;
    time_point_sec active;
    time_point_sec last_payout;

    uint8_t depth = 0;
    uint32_t children = 0;

    share_type net_rshares;
    share_type abs_rshares;
    share_type vote_rshares;

    share_type children_abs_rshares;
    time_point_sec cashout_time;
    uint64_t total_vote_weight = 0;

    asset total_payout_scr_value = asset(0, SCORUM_SYMBOL);
    asset author_payout_scr_value = asset(0, SCORUM_SYMBOL);
    asset curator_payout_scr_value = asset(0, SCORUM_SYMBOL);
    asset beneficiary_payout_scr_value = asset(0, SCORUM_SYMBOL);

    asset total_payout_sp_value = asset(0, SP_SYMBOL);
    asset author_payout_sp_value = asset(0, SP_SYMBOL);
    asset curator_payout_sp_value = asset(0, SP_SYMBOL);
    asset beneficiary_payout_sp_value = asset(0, SP_SYMBOL);

    int32_t net_votes = 0;

    comment_id_type root_comment;

    asset max_accepted_payout = asset(0, SCORUM_SYMBOL);
    bool allow_replies = false;
    bool allow_votes = false;
    bool allow_curation_rewards = false;
    std::vector<beneficiary_route_type> beneficiaries;

private:
    void set_comment(const chain::comment_object& o);
    void set_comment_statistic(const chain::comment_statistic_scr_object& stat);
    void set_comment_statistic(const chain::comment_statistic_sp_object& stat);
    void initialize(const chain::comment_object& o);
};

struct tag_api_obj
{
    tag_api_obj(const tag_stats_object& o)
        : name(o.tag)
        , total_payouts(o.total_payout)
        , net_votes(o.net_votes)
        , top_posts(o.top_posts)
        , comments(o.comments)
        , trending(o.total_trending)
    {
    }

    tag_api_obj()
    {
    }

    std::string name;
    asset total_payouts = asset(0, SCORUM_SYMBOL);
    int32_t net_votes = 0;
    uint32_t top_posts = 0;
    uint32_t comments = 0;
    fc::uint128 trending = 0;
};

struct discussion_index
{
    std::vector<std::string> trending; /// trending posts over the last 24 hours
    std::vector<std::string> payout; /// pending posts by payout
    std::vector<std::string> payout_comments; /// pending comments by payout
    std::vector<std::string> created; /// creation date
    std::vector<std::string> responses; /// creation date
    std::vector<std::string> active; /// last update or reply
    std::vector<std::string> votes; /// last update or reply
    std::vector<std::string> cashout; /// last update or reply
    std::vector<std::string> hot; /// total lifetime payout
    std::vector<std::string> promoted; /// pending lifetime payout
};

struct tag_index
{
    std::vector<std::string> trending; /// pending payouts
};

struct discussion : public comment_api_obj
{
    discussion(const comment_object& o)
        : comment_api_obj(o)
    {
    }

    discussion(const chain::comment_object& o,
               const comment_statistic_scr_service_i& stat_scr,
               const comment_statistic_sp_service_i& stat_sp)
        : comment_api_obj(o, stat_scr, stat_sp)
    {
    }

    discussion()
    {
    }

    std::string url; /// /category/@rootauthor/root_permlink#author/permlink
    std::string root_title;
    asset pending_payout_scr_value = asset(0, SCORUM_SYMBOL);
    asset pending_payout_sp_value = asset(0, SP_SYMBOL);
    std::vector<vote_state> active_votes;
    std::vector<std::string> replies; ///< author/slug mapping
    asset promoted = asset(0, SCORUM_SYMBOL);
    uint32_t body_length = 0;
    std::vector<account_name_type> reblogged_by;
    optional<account_name_type> first_reblogged_by;
    optional<time_point_sec> first_reblogged_on;
};

/**
 *  Defines the arguments to a query as a struct so it can be easily extended
 */
struct discussion_query
{
    void validate() const
    {
        FC_ASSERT(filter_tags.find(tag) == filter_tags.end());
        FC_ASSERT(limit <= 100);
    }

    std::string tag;
    uint32_t limit = 0;
    std::set<std::string> filter_tags;

    // list of authors to include, posts not by this author are filtered
    std::set<std::string> select_authors;

    // list of tags to include, posts without these tags are filtered
    std::set<std::string> select_tags;

    // the number of bytes of the post body to return, 0 for all
    uint32_t truncate_body = 0;

    optional<std::string> start_author;
    optional<std::string> start_permlink;
    optional<std::string> parent_author;
    optional<std::string> parent_permlink;
};

/**
 *  This struct is designed
 */
struct state
{
    std::string current_route;

    dynamic_global_property_api_obj props;

    tag_index tag_idx;

    /**
     * "" is the global discussion index
     */
    std::map<std::string, discussion_index> discussion_idx;

    std::map<std::string, tag_api_obj> tags;

    /**
     *  map from account/slug to full nested discussion
     */
    std::map<std::string, discussion> content;
    std::map<std::string, extended_account> accounts;

    /**
     * The list of block producers
     */
    std::map<std::string, witness_api_obj> witnesses;
    witness_schedule_api_obj witness_schedule;
    std::string error;
};

} // namespace api
} // namespace tags
} // namespace scorum

// clang-format off
FC_REFLECT(scorum::tags::api::tag_api_obj,
          (name)
          (total_payouts)
          (net_votes)
          (top_posts)
          (comments)
          (trending))

FC_REFLECT(scorum::tags::api::comment_api_obj,
          (id)
          (author)
          (permlink)
          (category)
          (parent_author)
          (parent_permlink)
          (title)
          (body)
          (json_metadata)
          (last_update)
          (created)
          (active)
          (last_payout)
          (depth)
          (children)
          (net_rshares)
          (abs_rshares)
          (vote_rshares)
          (children_abs_rshares)
          (cashout_time)
          (total_vote_weight)
          (total_payout_scr_value)
          (author_payout_scr_value)
          (curator_payout_scr_value)
          (beneficiary_payout_scr_value)
          (total_payout_sp_value)
          (author_payout_sp_value)
          (curator_payout_sp_value)
          (beneficiary_payout_sp_value)
          (net_votes)
          (root_comment)
          (max_accepted_payout)
          (allow_replies)
          (allow_votes)
          (allow_curation_rewards)
          (beneficiaries))

FC_REFLECT_DERIVED(scorum::tags::api::discussion, (scorum::tags::api::comment_api_obj),
                  (url)
                  (root_title)
                  (pending_payout_scr_value)
                  (pending_payout_sp_value)
                  (active_votes)
                  (replies)
                  (promoted)
                  (body_length)
                  (reblogged_by)
                  (first_reblogged_by)
                  (first_reblogged_on))

FC_REFLECT(scorum::tags::api::tag_index,
          (trending))

FC_REFLECT(scorum::tags::api::discussion_index,
          (trending)
          (payout)
          (payout_comments)
          (created)
          (responses)
          (active)
          (votes)
          (hot)
          (promoted)
          (cashout))

FC_REFLECT(scorum::tags::api::state,
          (current_route)
          (props)
          (tag_idx)
          (tags)
          (content)
          (accounts)
          (witnesses)
          (discussion_idx)
          (witness_schedule)
          (error))

FC_REFLECT(scorum::tags::api::discussion_query,
          (tag)
          (filter_tags)
          (select_tags)
          (select_authors)
          (truncate_body)
          (start_author)
          (start_permlink)
          (parent_author)
          (parent_permlink)
          (limit))
// clang-format on
