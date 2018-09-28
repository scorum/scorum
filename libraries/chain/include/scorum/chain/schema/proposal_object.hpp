#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/proposal_operations.hpp>
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using voted_accounts_type = fc::shared_flat_set<account_name_type>;

class proposal_object : public object<proposal_object_type, proposal_object>
{
public:
    using cref_type = std::reference_wrapper<const proposal_object>;

    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(proposal_object, (voted_accounts))

    id_type id;
    account_name_type creator;

    protocol::proposal_operation operation;

    fc::time_point_sec created;
    fc::time_point_sec expiration;

    protocol::percent_type quorum_percent = 0;

    voted_accounts_type voted_accounts;
};

struct by_expiration;
struct by_data;
struct by_created;

// clang-format off
typedef shared_multi_index_container<proposal_object,
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
                                                                   &proposal_object::created>>>
    >
    proposal_object_index;
// clang-format on

} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT( scorum::chain::voted_accounts_type, BOOST_PP_SEQ_NIL)

FC_REFLECT(scorum::chain::proposal_object,
           (id)
           (creator)
           (operation)
           (created)
           (expiration)
           (quorum_percent)
           (voted_accounts))
// clang-format on
CHAINBASE_SET_INDEX_TYPE(scorum::chain::proposal_object, scorum::chain::proposal_object_index)
