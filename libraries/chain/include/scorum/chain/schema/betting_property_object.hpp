#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::percent_type;

class betting_property_object : public object<betting_property_object_type, betting_property_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(betting_property_object)
    /// @endcond

    id_type id;

    account_name_type moderator;
    uint32_t resolve_delay_sec = SCORUM_BETTING_RESOLVE_DELAY_SEC;
};

struct by_moderator;

typedef shared_multi_index_container<betting_property_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<betting_property_object,
                                                                      betting_property_object::id_type,
                                                                      &betting_property_object::id>>,
                                                ordered_unique<tag<by_moderator>,
                                                               member<betting_property_object,
                                                                      account_name_type,
                                                                      &betting_property_object::moderator>>>>
    betting_property_index;
}
}

FC_REFLECT(scorum::chain::betting_property_object, (id)(moderator))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::betting_property_object, scorum::chain::betting_property_index)
