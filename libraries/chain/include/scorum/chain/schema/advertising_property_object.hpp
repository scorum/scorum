#pragma once
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::percent_type;

class advertising_property_object : public object<advertising_property_object_type, advertising_property_object>
{
public:
    /// \cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(advertising_property_object, (vcg_post_coefficients)(vcg_banner_coefficients))

    id_type id;

    account_name_type moderator;

    fc::shared_vector<percent_type> vcg_post_coefficients;
    fc::shared_vector<percent_type> vcg_banner_coefficients;
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

// clang-format off
FC_REFLECT(scorum::chain::advertising_property_object,
           (id)
           (moderator)
           (vcg_post_coefficients)
           (vcg_banner_coefficients))
// clang-format on

CHAINBASE_SET_INDEX_TYPE(scorum::chain::advertising_property_object, scorum::chain::advertising_property_index)
