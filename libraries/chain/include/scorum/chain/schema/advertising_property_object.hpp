#pragma once
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

class advertising_property_object : public object<advertising_property_object_type, advertising_property_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(advertising_property_object)

    id_type id;

    account_name_type moderator;
};

struct by_account;

// clang-format off
typedef shared_multi_index_container<advertising_property_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<advertising_property_object,
                                                                      advertising_property_object::id_type,
                                                                      &advertising_property_object::id>>,
                                                ordered_unique<tag<by_account>,
                                                               member<advertising_property_object,
                                                                      account_name_type,
                                                                      &advertising_property_object::moderator>>>>
    advertising_property_index;
// clang-format on
}
}

FC_REFLECT(scorum::chain::advertising_property_object, (id)(moderator))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::advertising_property_object, scorum::chain::advertising_property_index)