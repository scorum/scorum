#pragma once
#include <scorum/app/scorum_api_objects.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

namespace scorum {
namespace app {

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

} // namespace app
} // namespace scorum

// clang-format off

FC_REFLECT_DERIVED(scorum::app::extended_account,
                   (scorum::app::account_api_obj),
                   (witness_votes)
                   (tags_usage)
                   (comments)
                   (recent_replies))

FC_REFLECT(scorum::app::vote_state,
           (voter)
           (weight)
           (rshares)
           (percent)
           (time))

FC_REFLECT(scorum::app::account_vote,
           (authorperm)
           (weight)
           (rshares)
           (percent)
           (time))

// clang-format on
