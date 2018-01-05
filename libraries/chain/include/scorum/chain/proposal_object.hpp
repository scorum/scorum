#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/chain/scorum_object_types.hpp>

namespace scorum {
namespace chain {

class proposal_object : public object<proposal_object_type, proposal_object>
{
public:
    using ref_type = std::reference_wrapper<const proposal_object>;

    template <typename Constructor, typename Allocator> proposal_object(Constructor&& c, allocator<Allocator>)
    {
        c(*this);
    }

    proposal_object()
        : action(scorum::protocol::proposal_action::invite)
    {
    }

    id_type id;
    account_name_type creator;
    fc::variant data;

    fc::time_point_sec expiration;

    uint64_t quorum_percent = 0;

    scorum::protocol::proposal_action action;
    flat_set<account_name_type> voted_accounts;
};

struct by_expiration;

typedef multi_index_container<proposal_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<proposal_object,
                                                               proposal_id_type,
                                                               &proposal_object::id>>,
                                         ordered_unique<tag<by_expiration>,
                                                        member<proposal_object,
                                                               fc::time_point_sec,
                                                               &proposal_object::expiration>>>,
                              allocator<proposal_object>>
    proposal_object_index;

} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::proposal_object, (id)(creator)(data)(expiration)(quorum_percent)(action)(voted_accounts))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::proposal_object, scorum::chain::proposal_object_index)
