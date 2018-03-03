#pragma once

#include <boost/multi_index/composite_key.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//

#ifndef ACCOUNT_HISTORY_SPACE_ID
#define ACCOUNT_HISTORY_SPACE_ID 5
#endif

namespace scorum {
namespace account_history {

using namespace scorum::chain;

enum history_object_type
{
    account_history_object_type = (ACCOUNT_HISTORY_SPACE_ID << 8)
};

class account_history_object : public object<account_history_object_type, account_history_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(account_history_object)

    id_type id;

    account_name_type account;
    uint32_t sequence = 0;
    operation_id_type op;
};

using account_history_id_type = oid<account_history_object>;

struct by_account;
typedef shared_multi_index_container<account_history_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<account_history_object,
                                                                      account_history_id_type,
                                                                      &account_history_object::id>>,
                                                ordered_unique<tag<by_account>,
                                                               composite_key<account_history_object,
                                                                             member<account_history_object,
                                                                                    account_name_type,
                                                                                    &account_history_object::account>,
                                                                             member<account_history_object,
                                                                                    uint32_t,
                                                                                    &account_history_object::sequence>>,
                                                               composite_key_compare<std::less<account_name_type>,
                                                                                     std::greater<uint32_t>>>>>
    account_history_index;
} // namespace account_history
} // namespace scorum

FC_REFLECT(scorum::account_history::account_history_object, (id)(account)(sequence)(op))
CHAINBASE_SET_INDEX_TYPE(scorum::account_history::account_history_object,
                         scorum::account_history::account_history_index)
