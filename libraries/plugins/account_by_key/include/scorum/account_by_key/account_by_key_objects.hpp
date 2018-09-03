#pragma once
#include <scorum/chain/schema/scorum_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace scorum {
namespace account_by_key {

using namespace scorum::chain;

#ifndef ACCOUNT_BY_KEY_SPACE_ID
#define ACCOUNT_BY_KEY_SPACE_ID 2
#endif

enum account_by_key_object_types
{
    key_lookup_object_type = (ACCOUNT_BY_KEY_SPACE_ID << OBJECT_TYPE_SPACE_ID_OFFSET)
};

class key_lookup_object : public object<key_lookup_object_type, key_lookup_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(key_lookup_object)

    id_type id;

    public_key_type key;
    account_name_type account;
};

typedef key_lookup_object::id_type key_lookup_id_type;

using namespace boost::multi_index;

struct by_key;

typedef shared_multi_index_container<key_lookup_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<key_lookup_object,
                                                                      key_lookup_id_type,
                                                                      &key_lookup_object::id>>,
                                                ordered_unique<tag<by_key>,
                                                               composite_key<key_lookup_object,
                                                                             member<key_lookup_object,
                                                                                    public_key_type,
                                                                                    &key_lookup_object::key>,
                                                                             member<key_lookup_object,
                                                                                    account_name_type,
                                                                                    &key_lookup_object::account>>>>>
    key_lookup_index;
}
} // scorum::account_by_key

FC_REFLECT(scorum::account_by_key::key_lookup_object, (id)(key)(account))
CHAINBASE_SET_INDEX_TYPE(scorum::account_by_key::key_lookup_object, scorum::account_by_key::key_lookup_index)
