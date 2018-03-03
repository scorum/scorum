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

enum account_history_object_type
{
    all_operations_history = (ACCOUNT_HISTORY_SPACE_ID << 8),
    scr_to_scr_transfers_history,
    scr_to_sp_transfers_history
};

template <uint16_t HistoryType> struct history_object : public object<HistoryType, history_object<HistoryType>>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(history_object)

    typedef typename object<HistoryType, history_object<HistoryType>>::id_type id_type;

    id_type id;

    account_name_type account;
    uint32_t sequence = 0;
    operation_id_type op;
};

struct by_account;

template <typename history_object_t>
using history_index
    = shared_multi_index_container<history_object_t,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<history_object_t,
                                                                    typename history_object_t::id_type,
                                                                    &history_object_t::id>>,
                                              ordered_unique<tag<by_account>,
                                                             composite_key<history_object_t,
                                                                           member<history_object_t,
                                                                                  account_name_type,
                                                                                  &history_object_t::account>,
                                                                           member<history_object_t,
                                                                                  uint32_t,
                                                                                  &history_object_t::sequence>>,
                                                             composite_key_compare<std::less<account_name_type>,
                                                                                   std::greater<uint32_t>>>>>;

using account_history_object = history_object<all_operations_history>;
using transfers_to_scr_history_object = history_object<scr_to_scr_transfers_history>;
using transfers_to_sp_history_object = history_object<scr_to_sp_transfers_history>;

using account_full_history_index = history_index<account_history_object>;
using transfers_to_scr_history_index = history_index<transfers_to_scr_history_object>;
using transfers_to_sp_history_index = history_index<transfers_to_sp_history_object>;
//
} // namespace account_history
} // namespace scorum

FC_REFLECT(scorum::account_history::account_history_object, (id)(account)(sequence)(op))
FC_REFLECT(scorum::account_history::transfers_to_scr_history_object, (id)(account)(sequence)(op))
FC_REFLECT(scorum::account_history::transfers_to_sp_history_object, (id)(account)(sequence)(op))

CHAINBASE_SET_INDEX_TYPE(scorum::account_history::account_history_object,
                         scorum::account_history::account_full_history_index)

CHAINBASE_SET_INDEX_TYPE(scorum::account_history::transfers_to_scr_history_object,
                         scorum::account_history::transfers_to_scr_history_index)

CHAINBASE_SET_INDEX_TYPE(scorum::account_history::transfers_to_sp_history_object,
                         scorum::account_history::transfers_to_sp_history_index)
