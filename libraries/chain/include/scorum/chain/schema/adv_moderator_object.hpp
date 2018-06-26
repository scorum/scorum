#pragma once
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

class adv_moderator_object : public object<adv_moderator_object_type, adv_moderator_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(adv_moderator_object)

    id_type id;

    account_id_type account_id;
    account_name_type account_name;
};

struct by_account;

// clang-format off
typedef shared_multi_index_container<adv_moderator_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<adv_moderator_object,
                                                                      adv_moderator_object::id_type,
                                                                      &adv_moderator_object::id>>,
                                                ordered_unique<tag<by_account>,
                                                               composite_key<adv_moderator_object,
                                                                             member<adv_moderator_object,
                                                                                    account_name_type,
                                                                                    &comment_object::account_name>,
                                                                             member<adv_moderator_object,
                                                                                    account_id_type,
                                                                                    &adv_moderator_object::account_id>>>>>
    adv_moderator_index;

FC_REFLECT(scorum::chain::adv_moderator_object,
           (id)
           (account_id)
           (account_name))
// clang-format on

CHAINBASE_SET_INDEX_TYPE(scorum::chain::adv_moderator_object, scorum::chain::adv_moderator_index)
}
}