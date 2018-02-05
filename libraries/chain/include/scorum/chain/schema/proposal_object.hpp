#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

class proposal_object : public object<proposal_object_type, proposal_object>
{
public:
    using cref_type = std::reference_wrapper<const proposal_object>;

    CHAINBASE_DEFAULT_CONSTRUCTOR(proposal_object)

    id_type id;
    account_name_type creator;
    fc::variant data;

    fc::time_point_sec created;
    fc::time_point_sec expiration;

    uint64_t quorum_percent = 0;

    scorum::protocol::proposal_action action = scorum::protocol::proposal_action::invite;
    flat_set<account_name_type> voted_accounts;
};

struct by_expiration;
struct by_data;
struct by_created;

// clang-format off
typedef multi_index_container<proposal_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<proposal_object,
                                                               proposal_id_type,
                                                               &proposal_object::id>>,
                                         ordered_non_unique<tag<by_expiration>,
                                                            member<proposal_object,
                                                                   fc::time_point_sec,
                                                                   &proposal_object::expiration>>,
                                         ordered_non_unique<tag<by_created>,
                                                            member<proposal_object,
                                                                   fc::time_point_sec,
                                                                   &proposal_object::created>>>,
                              fc::shared_allocator<proposal_object>>
    proposal_object_index;
// clang-format on

} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::proposal_object,
           (id)(creator)(data)(created)(expiration)(quorum_percent)(action)(voted_accounts))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::proposal_object, scorum::chain::proposal_object_index)
