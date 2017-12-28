#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/chain/scorum_object_types.hpp>

namespace scorum {
namespace chain {

class proposal_vote_object : public object<proposal_vote_object_type, proposal_vote_object>
{
public:
    template <typename Constructor, typename Allocator> proposal_vote_object(Constructor&& c, allocator<Allocator>)
    {
        c(*this);
    }

    proposal_vote_object()
        : action(scorum::protocol::proposal_action::invite)
        , votes(0)
    {
    }

    id_type id;
    account_name_type creator;
    account_name_type member;

    fc::time_point_sec expiration;

    scorum::protocol::proposal_action action;
    uint32_t votes;
};

struct by_member_name;
struct by_expiration;

typedef multi_index_container<proposal_vote_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<proposal_vote_object,
                                                               proposal_id_type,
                                                               &proposal_vote_object::id>>,
                                         ordered_unique<tag<by_member_name>,
                                                        member<proposal_vote_object,
                                                               account_name_type,
                                                               &proposal_vote_object::member>>,
                                         ordered_unique<tag<by_expiration>,
                                                        member<proposal_vote_object,
                                                               fc::time_point_sec,
                                                               &proposal_vote_object::expiration>>>,
                              allocator<proposal_vote_object>>
    proposal_vote_index;

} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::proposal_vote_object, (id)(creator)(member)(action)(expiration)(votes))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::proposal_vote_object, scorum::chain::proposal_vote_index)
