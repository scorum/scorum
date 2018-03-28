#pragma once
#include <scorum/app/scorum_api_objects.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

namespace scorum {
namespace app {

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

struct vote_state
{
    std::string voter;
    uint64_t weight = 0;
    int64_t rshares = 0;
    int16_t percent = 0;
    time_point_sec time;
};

struct account_vote
{
    std::string authorperm;
    uint64_t weight = 0;
    int64_t rshares = 0;
    int16_t percent = 0;
    time_point_sec time;
};

struct discussion : public comment_api_obj
{
    discussion(const comment_object& o)
        : comment_api_obj(o)
    {
    }
    discussion()
    {
    }

    std::string url; /// /category/@rootauthor/root_permlink#author/permlink
    std::string root_title;
    asset pending_payout_value = asset(0, SCORUM_SYMBOL);
    std::vector<vote_state> active_votes;
    std::vector<std::string> replies; ///< author/slug mapping
    asset promoted = asset(0, SCORUM_SYMBOL);
    uint32_t body_length = 0;
    std::vector<account_name_type> reblogged_by;
    optional<account_name_type> first_reblogged_by;
    optional<time_point_sec> first_reblogged_on;
};

/**
 *  Convert's scorumpower shares
 */
struct extended_account : public account_api_obj
{
    extended_account()
    {
    }
    extended_account(const account_object& a, const database& db)
        : account_api_obj(a, db)
    {
    }

    //    std::map<uint64_t, applied_operation> transfer_history; /// transfer to/from scorumpower
    //    std::map<uint64_t, applied_operation> post_history;
    //    std::map<uint64_t, applied_operation> vote_history;
    //    std::map<uint64_t, applied_operation> other_history;
    std::set<std::string> witness_votes;
    std::vector<std::pair<std::string, uint32_t>> tags_usage;

    optional<std::vector<std::string>> comments; /// permlinks for this user
    optional<std::vector<std::string>> recent_replies; /// blog posts for this user
};

/**
 *  This struct is designed
 */
struct state
{
    std::string current_route;

    dynamic_global_property_api_obj props;

    app::tag_index tag_idx;

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
}
}

// clang-format off

FC_REFLECT_DERIVED( scorum::app::extended_account,
                   (scorum::app::account_api_obj),
                   (witness_votes)(tags_usage)(comments)(recent_replies) )


FC_REFLECT( scorum::app::vote_state, (voter)(weight)(rshares)(percent)(time) )
FC_REFLECT( scorum::app::account_vote, (authorperm)(weight)(rshares)(percent)(time) )

FC_REFLECT( scorum::app::discussion_index, (trending)(payout)(payout_comments)(created)(responses)(active)(votes)(hot)(promoted)(cashout) )
FC_REFLECT( scorum::app::tag_index, (trending) )
FC_REFLECT_DERIVED( scorum::app::discussion, (scorum::app::comment_api_obj), (url)(root_title)(pending_payout_value)(active_votes)(replies)(promoted)(body_length)(reblogged_by)(first_reblogged_by)(first_reblogged_on) )

FC_REFLECT( scorum::app::state, (current_route)(props)(tag_idx)(tags)(content)(accounts)(witnesses)(discussion_idx)(witness_schedule)(error) )

// clang-format on
