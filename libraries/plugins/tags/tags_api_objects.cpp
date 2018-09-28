#include <scorum/tags/tags_api_objects.hpp>

#include <scorum/chain/services/comment_statistic.hpp>

namespace scorum {
namespace tags {
namespace api {

comment_api_obj::comment_api_obj(const chain::comment_object& o)
{
    set_comment(o);
    initialize(o);
}

comment_api_obj::comment_api_obj(const chain::comment_object& o,
                                 const comment_statistic_scr_service_i& statistic_scr_service,
                                 const comment_statistic_sp_service_i& statistic_sp_service)
{
    set_comment(o);
    set_comment_statistic(statistic_scr_service.get(o.id));
    set_comment_statistic(statistic_sp_service.get(o.id));
    initialize(o);
}

void comment_api_obj::set_comment(const chain::comment_object& o)
{
    id = o.id;
    category = fc::to_string(o.category);
    parent_author = o.parent_author;
    parent_permlink = fc::to_string(o.parent_permlink);
    author = o.author;
    permlink = fc::to_string(o.permlink);
    title = fc::to_string(o.title);
    body = fc::to_string(o.body);
    json_metadata = fc::to_string(o.json_metadata);
    last_update = o.last_update;
    created = o.created;
    active = o.active;
    last_payout = o.last_payout;
    depth = o.depth;
    children = o.children;
    net_rshares = o.net_rshares;
    abs_rshares = o.abs_rshares;
    vote_rshares = o.vote_rshares;
    children_abs_rshares = o.children_abs_rshares;
    cashout_time = o.cashout_time;
    total_vote_weight = o.total_vote_weight;
    net_votes = o.net_votes;
    root_comment = o.root_comment;
    max_accepted_payout = o.max_accepted_payout;
    allow_replies = o.allow_replies;
    allow_votes = o.allow_votes;
    allow_curation_rewards = o.allow_curation_rewards;
}

void comment_api_obj::set_comment_statistic(const chain::comment_statistic_scr_object& stat)
{
    total_payout_scr_value = stat.total_payout_value;
    author_payout_scr_value = stat.author_payout_value;
    curator_payout_scr_value = stat.curator_payout_value;
    from_children_payout_scr_value = stat.from_children_payout_value;
    beneficiary_payout_scr_value = stat.beneficiary_payout_value;
    to_parent_payout_scr_value = stat.to_parent_payout_value;
}

void comment_api_obj::set_comment_statistic(const chain::comment_statistic_sp_object& stat)
{
    total_payout_sp_value = stat.total_payout_value;
    author_payout_sp_value = stat.author_payout_value;
    curator_payout_sp_value = stat.curator_payout_value;
    from_children_payout_sp_value = stat.from_children_payout_value;
    beneficiary_payout_sp_value = stat.beneficiary_payout_value;
    to_parent_payout_sp_value = stat.to_parent_payout_value;
}

void comment_api_obj::initialize(const chain::comment_object& o)
{
    for (auto& route : o.beneficiaries)
    {
        beneficiaries.push_back(route);
    }
}
}
}
}
