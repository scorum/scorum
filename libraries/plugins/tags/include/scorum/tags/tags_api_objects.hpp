#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/common_api/config.hpp>

#include <scorum/chain/services/comment_statistic.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

#include <scorum/app/scorum_api_objects.hpp>
#include <scorum/app/state.hpp>

#include <scorum/tags/tags_objects.hpp>

namespace scorum {
namespace tags {
namespace api {

using scorum::protocol::asset;
using scorum::protocol::account_name_type;
using scorum::protocol::share_type;

using scorum::chain::comment_id_type;
using scorum::chain::beneficiary_route_type;
using scorum::chain::comment_object;

using scorum::app::dynamic_global_property_api_obj;
using scorum::app::vote_state;

/// @addtogroup tags_api
/// @{

struct comment_api_obj
{
    comment_api_obj()
    {
    }

    comment_api_obj(const scorum::chain::comment_object& o);

    comment_api_obj(const chain::comment_object& o,
                    const comment_statistic_scr_service_i&,
                    const comment_statistic_sp_service_i&);

    comment_id_type root_comment;

    comment_id_type id;
    std::string category;

    account_name_type parent_author;
    std::string parent_permlink;

    account_name_type author;
    std::string permlink;

    std::string title;
    std::string body;
    std::string json_metadata;

    fc::time_point_sec last_update;
    fc::time_point_sec created;
    fc::time_point_sec active;
    fc::time_point_sec last_payout;
    fc::time_point_sec cashout_time;

    uint8_t depth = 0;
    uint32_t children = 0;
    int32_t net_votes = 0;
    uint64_t total_vote_weight = 0;

    share_type children_abs_rshares;

    share_type net_rshares;
    share_type abs_rshares;
    share_type vote_rshares;

    asset total_payout_scr_value = asset(0, SCORUM_SYMBOL);
    asset author_payout_scr_value = asset(0, SCORUM_SYMBOL);
    asset curator_payout_scr_value = asset(0, SCORUM_SYMBOL);
    asset beneficiary_payout_scr_value = asset(0, SCORUM_SYMBOL);

    asset total_payout_sp_value = asset(0, SP_SYMBOL);
    asset author_payout_sp_value = asset(0, SP_SYMBOL);
    asset curator_payout_sp_value = asset(0, SP_SYMBOL);
    asset beneficiary_payout_sp_value = asset(0, SP_SYMBOL);

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
        , total_payouts_scr(o.total_payout_scr)
        , total_payouts_sp(o.total_payout_sp)
        , net_votes(o.net_votes)
        , posts(o.posts)
        , trending(o.total_trending)
    {
    }

    tag_api_obj()
    {
    }

    std::string name;
    asset total_payouts_scr = asset(0, SCORUM_SYMBOL);
    asset total_payouts_sp = asset(0, SP_SYMBOL);
    int32_t net_votes = 0;
    uint32_t posts = 0;
    fc::uint128 trending = 0;
};

struct discussion : public comment_api_obj
{
    discussion(const chain::comment_object& o,
               const comment_statistic_scr_service_i& stat_scr,
               const comment_statistic_sp_service_i& stat_sp)
        : comment_api_obj(o, stat_scr, stat_sp)
    {
    }

    discussion()
    {
    }

    /// /category/@rootauthor/root_permlink#author/permlink
    std::string url;
    std::string root_title;

    asset pending_payout_scr = asset(0, SCORUM_SYMBOL);
    asset pending_payout_sp = asset(0, SP_SYMBOL);
    asset promoted = asset(0, SCORUM_SYMBOL);

    std::vector<vote_state> active_votes;
    std::vector<std::string> replies; ///< author/slug mapping

    uint32_t body_length = 0;
};

struct discussion_query
{
    /// the number of bytes of the post body to return, 0 for all
    uint32_t truncate_body = 0;

    /// start author
    optional<std::string> start_author;

    /// start permlink
    optional<std::string> start_permlink;

    /// query limit
    uint32_t limit = 0;

    bool tags_logical_and = true;
    std::set<std::string> tags;
};

/// @}

} // namespace api
} // namespace tags
} // namespace scorum

// clang-format off
FC_REFLECT(scorum::tags::api::tag_api_obj,
          (name)
          (total_payouts_scr)
          (total_payouts_sp)
          (net_votes)
          (posts)
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
                  (pending_payout_scr)
                  (pending_payout_sp)
                  (active_votes)
                  (replies)
                  (promoted)
                  (body_length))

FC_REFLECT(scorum::tags::api::discussion_query,
          (truncate_body)
          (start_author)
          (start_permlink)
          (limit)
          (tags)
          (tags_logical_and))
// clang-format on
