#pragma once

#include <fc/shared_string.hpp>
#include <fc/shared_buffer.hpp>

#include <scorum/protocol/authority.hpp>
#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <limits>

namespace scorum {
namespace chain {

using scorum::protocol::beneficiary_route_type;
using scorum::protocol::vote_weight_type;

class comment_object : public object<comment_object_type, comment_object>
{
public:
    /// \cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(
        comment_object, (category)(parent_permlink)(permlink)(title)(body)(json_metadata)(beneficiaries))

    id_type id;

    fc::shared_string category;
    account_name_type parent_author;
    fc::shared_string parent_permlink;
    account_name_type author;
    fc::shared_string permlink;

    fc::shared_string title;
    fc::shared_string body;
    fc::shared_string json_metadata;
    time_point_sec last_update;
    time_point_sec created;

    /// the last time this post was "touched" by voting or reply
    time_point_sec active;

    time_point_sec last_payout;

    /// used to track max nested depth
    uint16_t depth = 0;

    /// used to track the total number of children, grandchildren, etc...
    uint32_t children = 0;

    /// This is the sum of all votes (positive and negative). Used for calculation reward witch is proportional to
    /// rshares^2
    share_type net_rshares;

    /// This is used to track the total abs(weight) of votes.
    share_type abs_rshares;

    /// Total positive rshares from all votes. Used to calculate delta weights. Needed to handle vote changing and
    /// removal.
    share_type vote_rshares;

    /// this is used to calculate cashout time of a discussion.
    share_type children_abs_rshares;

    /// 24 hours from the weighted average of vote time
    time_point_sec cashout_time;

    /// the total weight of voting rewards, used to calculate pro-rata share of curation payouts
    uint64_t total_vote_weight = 0;

    int32_t net_votes = 0;

    id_type root_comment;

    /// SCR value of the maximum payout this post will receive
    asset max_accepted_payout = asset::maximum(SCORUM_SYMBOL);

    /// allows a post to disable replies.
    bool allow_replies = true;

    /// allows a post to receive votes;
    bool allow_votes = true;

    bool allow_curation_rewards = true;

    fc::shared_vector<beneficiary_route_type> beneficiaries;
};

/**
 * This index maintains the set of voter/comment pairs that have been used, voters cannot
 * vote on the same comment more than once per payout period.
 */
class comment_vote_object : public object<comment_vote_object_type, comment_vote_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(comment_vote_object)

    id_type id;

    account_id_type voter;
    comment_id_type comment;

    /// defines the score this vote receives, used by vote payout calc. 0 if a negative vote or changed votes.
    uint64_t weight = 0;

    /// The number of rshares this vote is responsible for
    share_type rshares;

    /// The percent weight of the vote
    vote_weight_type vote_percent = 0;

    /// The time of the last update of the vote
    time_point_sec last_update;
    int8_t num_changes = 0;
};

template <uint16_t ObjectType, asset_symbol_type SymbolType>
class comment_statistic_object : public object<ObjectType, comment_statistic_object<ObjectType, SymbolType>>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(comment_statistic_object)

    typedef typename object<ObjectType, comment_statistic_object<ObjectType, SymbolType>>::id_type id_type;

    id_type id;

    comment_id_type comment;

    /// tracks the total payout this comment has received over time
    asset total_payout_value = asset(0, SymbolType);

    asset author_payout_value = asset(0, SymbolType);

    asset curator_payout_value = asset(0, SymbolType);

    asset beneficiary_payout_value = asset(0, SymbolType);

    /// the payout for the comment/post publication
    asset comment_publication_reward = asset(0, SymbolType);

    /// the payout obtained from children comments' rewards
    asset children_comments_reward = asset(0, SymbolType);
};

using comment_statistic_scr_object = comment_statistic_object<comment_statistic_scr_object_type, SCORUM_SYMBOL>;
using comment_statistic_sp_object = comment_statistic_object<comment_statistic_sp_object_type, SP_SYMBOL>;

struct by_created;
struct by_cashout_time;
struct by_permlink;
struct by_root;
struct by_parent;
struct by_last_update;
struct by_author_created;

typedef shared_multi_index_container<comment_object,
                                     indexed_by<
                                         /// CONSENUSS INDICIES - used by evaluators
                                         ordered_unique<tag<by_id>,
                                                        member<comment_object, comment_id_type, &comment_object::id>>,
                                         ordered_unique<tag<by_created>,
                                                        composite_key<comment_object,
                                                                      member<comment_object,
                                                                             time_point_sec,
                                                                             &comment_object::created>,
                                                                      member<comment_object,
                                                                             comment_id_type,
                                                                             &comment_object::id>>>,
                                         ordered_unique<tag<by_cashout_time>,
                                                        composite_key<comment_object,
                                                                      member<comment_object,
                                                                             time_point_sec,
                                                                             &comment_object::cashout_time>,
                                                                      member<comment_object,
                                                                             comment_id_type,
                                                                             &comment_object::id>>>,
                                         ordered_unique<tag<by_permlink>, /// used by consensus to find posts referenced
                                                        /// in ops
                                                        composite_key<comment_object,
                                                                      member<comment_object,
                                                                             account_name_type,
                                                                             &comment_object::author>,
                                                                      member<comment_object,
                                                                             fc::shared_string,
                                                                             &comment_object::permlink>>,
                                                        composite_key_compare<std::less<account_name_type>,
                                                                              fc::strcmp_less>>,
                                         ordered_unique<tag<by_root>,
                                                        composite_key<comment_object,
                                                                      member<comment_object,
                                                                             comment_id_type,
                                                                             &comment_object::root_comment>,
                                                                      member<comment_object,
                                                                             comment_id_type,
                                                                             &comment_object::id>>>,
                                         ordered_unique<tag<by_parent>, /// used by consensus to find posts referenced
                                                        /// in ops
                                                        composite_key<comment_object,
                                                                      member<comment_object,
                                                                             account_name_type,
                                                                             &comment_object::parent_author>,
                                                                      member<comment_object,
                                                                             fc::shared_string,
                                                                             &comment_object::parent_permlink>,
                                                                      member<comment_object,
                                                                             comment_id_type,
                                                                             &comment_object::id>>,
                                                        composite_key_compare<std::less<account_name_type>,
                                                                              fc::strcmp_less,
                                                                              std::less<comment_id_type>>>
/// NON_CONSENSUS INDICIES - used by APIs
#ifndef IS_LOW_MEM
                                         ,
                                         ordered_unique<tag<by_author_created>,
                                                        composite_key<comment_object,
                                                                      member<comment_object,
                                                                             account_name_type,
                                                                             &comment_object::author>,
                                                                      member<comment_object,
                                                                             time_point_sec,
                                                                             &comment_object::created>,
                                                                      member<comment_object,
                                                                             comment_id_type,
                                                                             &comment_object::id>>,
                                                        composite_key_compare<std::less<account_name_type>,
                                                                              std::greater<time_point_sec>,
                                                                              std::less<comment_id_type>>>
#endif
                                         >>
    comment_index;

struct by_comment_voter;
struct by_voter_comment;
struct by_comment_weight_voter;
struct by_voter_last_update;
typedef shared_multi_index_container<comment_vote_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<comment_vote_object,
                                                                      comment_vote_id_type,
                                                                      &comment_vote_object::id>>,
                                                ordered_unique<tag<by_comment_voter>,
                                                               composite_key<comment_vote_object,
                                                                             member<comment_vote_object,
                                                                                    comment_id_type,
                                                                                    &comment_vote_object::comment>,
                                                                             member<comment_vote_object,
                                                                                    account_id_type,
                                                                                    &comment_vote_object::voter>>>,
                                                ordered_unique<tag<by_voter_comment>,
                                                               composite_key<comment_vote_object,
                                                                             member<comment_vote_object,
                                                                                    account_id_type,
                                                                                    &comment_vote_object::voter>,
                                                                             member<comment_vote_object,
                                                                                    comment_id_type,
                                                                                    &comment_vote_object::comment>>>,
                                                ordered_unique<tag<by_voter_last_update>,
                                                               composite_key<comment_vote_object,
                                                                             member<comment_vote_object,
                                                                                    account_id_type,
                                                                                    &comment_vote_object::voter>,
                                                                             member<comment_vote_object,
                                                                                    time_point_sec,
                                                                                    &comment_vote_object::last_update>,
                                                                             member<comment_vote_object,
                                                                                    comment_id_type,
                                                                                    &comment_vote_object::comment>>,
                                                               composite_key_compare<std::less<account_id_type>,
                                                                                     std::greater<time_point_sec>,
                                                                                     std::less<comment_id_type>>>,
                                                ordered_unique<tag<by_comment_weight_voter>,
                                                               composite_key<comment_vote_object,
                                                                             member<comment_vote_object,
                                                                                    comment_id_type,
                                                                                    &comment_vote_object::comment>,
                                                                             member<comment_vote_object,
                                                                                    uint64_t,
                                                                                    &comment_vote_object::weight>,
                                                                             member<comment_vote_object,
                                                                                    account_id_type,
                                                                                    &comment_vote_object::voter>>,
                                                               composite_key_compare<std::less<comment_id_type>,
                                                                                     std::greater<uint64_t>,
                                                                                     std::less<account_id_type>>>>>
    comment_vote_index;

struct by_comment_id;

template <typename CommentStatisticObjectType>
using comment_statistic_index
    = shared_multi_index_container<CommentStatisticObjectType,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<CommentStatisticObjectType,
                                                                    typename CommentStatisticObjectType::id_type,
                                                                    &CommentStatisticObjectType::id>>,
                                              ordered_unique<tag<by_comment_id>,
                                                             member<CommentStatisticObjectType,
                                                                    comment_id_type,
                                                                    &CommentStatisticObjectType::comment>>>>;

using comment_statistic_scr_index = comment_statistic_index<comment_statistic_scr_object>;
using comment_statistic_sp_index = comment_statistic_index<comment_statistic_sp_object>;

} // namespace chain
} // namespace scorum

// clang-format off

FC_REFLECT( scorum::chain::comment_object,
             (id)(author)(permlink)
             (category)(parent_author)(parent_permlink)
             (title)(body)(json_metadata)(last_update)(created)(active)(last_payout)
             (depth)(children)
             (net_rshares)(abs_rshares)(vote_rshares)
             (children_abs_rshares)(cashout_time)
             (total_vote_weight)(net_votes)(root_comment)
             (max_accepted_payout)(allow_replies)(allow_votes)(allow_curation_rewards)
             (beneficiaries)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::comment_object, scorum::chain::comment_index )

FC_REFLECT( scorum::chain::comment_vote_object,
             (id)(voter)(comment)(weight)(rshares)(vote_percent)(last_update)(num_changes)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::comment_vote_object, scorum::chain::comment_vote_index )

FC_REFLECT( scorum::chain::comment_statistic_scr_object,
             (id)(comment)
            (total_payout_value)
            (author_payout_value)
            (curator_payout_value)
            (beneficiary_payout_value)
            (comment_publication_reward)
            (children_comments_reward)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::comment_statistic_scr_object, scorum::chain::comment_statistic_scr_index )

FC_REFLECT( scorum::chain::comment_statistic_sp_object,
             (id)(comment)
            (total_payout_value)
            (author_payout_value)
            (curator_payout_value)
            (beneficiary_payout_value)
            (comment_publication_reward)
            (children_comments_reward)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::comment_statistic_sp_object, scorum::chain::comment_statistic_sp_index )

// clang-format on
