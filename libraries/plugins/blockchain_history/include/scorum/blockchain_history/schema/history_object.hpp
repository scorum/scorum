#pragma once

#include <scorum/blockchain_history/schema/operation_objects.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace scorum {
namespace blockchain_history {
namespace impl {

template <uint16_t HistoryType>
struct account_history_object : public object<HistoryType, account_history_object<HistoryType>>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(account_history_object)

    typedef typename object<HistoryType, account_history_object<HistoryType>>::id_type id_type;

    id_type id;

    account_name_type account;
    uint32_t sequence = 0;
    operation_object::id_type op;
};

template <uint16_t HistoryType>
struct withdrawals_account_history_object : public object<HistoryType, withdrawals_account_history_object<HistoryType>>
{
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(withdrawals_account_history_object, (progress))

    typedef typename object<HistoryType, withdrawals_account_history_object<HistoryType>>::id_type id_type;

    id_type id;

    account_name_type account;
    uint32_t sequence = 0;
    operation_object::id_type op;

    fc::shared_vector<operation_object::id_type> progress;
};

template <uint16_t HistoryType>
struct devcommittee_history_object : public object<HistoryType, devcommittee_history_object<HistoryType>>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(devcommittee_history_object)

    typedef typename object<HistoryType, devcommittee_history_object<HistoryType>>::id_type id_type;

    id_type id;
    operation_object::id_type op;
};

template <uint16_t HistoryType>
struct withdrawals_devcommittee_history_object
    : public object<HistoryType, withdrawals_devcommittee_history_object<HistoryType>>
{
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(withdrawals_devcommittee_history_object, (progress))

    typedef typename object<HistoryType, withdrawals_devcommittee_history_object<HistoryType>>::id_type id_type;

    id_type id;
    operation_object::id_type op;
    fc::shared_vector<operation_object::id_type> progress;
};
}

struct by_account;
struct by_id_desc;

template <typename history_object_t>
using account_history_index
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
                                                                                   // for derect oder iteration from
                                                                                   // grater value to less
                                                                                   std::greater<uint32_t>>>>>;

template <typename history_object_t>
using devcommittee_history_index
    = shared_multi_index_container<history_object_t,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<history_object_t,
                                                                    typename history_object_t::id_type,
                                                                    &history_object_t::id>>,
                                              ordered_unique<tag<by_id_desc>,
                                                             member<history_object_t,
                                                                    typename history_object_t::id_type,
                                                                    &history_object_t::id>,
                                                             std::greater<typename history_object_t::id_type>>>>;

using account_history_object = impl::account_history_object<account_all_operations_history_object_type>;
using account_transfers_to_scr_history_object
    = impl::account_history_object<account_scr_to_scr_transfers_history_object_type>;
using account_transfers_to_sp_history_object
    = impl::account_history_object<account_scr_to_sp_transfers_history_object_type>;
using account_withdrawals_to_scr_history_object
    = impl::withdrawals_account_history_object<account_sp_to_scr_withdrawals_history_object_type>;

using devcommittee_history_object = impl::devcommittee_history_object<devcommittee_all_operations_history>;
using devcommittee_transfers_to_scr_history_object
    = impl::devcommittee_history_object<devcommittee_scr_to_scr_transfers_history>;
using devcommittee_withdrawals_to_scr_history_object
    = impl::withdrawals_devcommittee_history_object<devcommittee_sp_to_scr_withdrawals_history>;

using account_operations_full_history_index = account_history_index<account_history_object>;
using account_transfers_to_scr_history_index = account_history_index<account_transfers_to_scr_history_object>;
using account_transfers_to_sp_history_index = account_history_index<account_transfers_to_sp_history_object>;
using account_withdrawals_to_scr_history_index = account_history_index<account_withdrawals_to_scr_history_object>;

using devcommittee_operations_full_history_index = devcommittee_history_index<devcommittee_history_object>;
using devcommittee_transfers_to_scr_history_index
    = devcommittee_history_index<devcommittee_transfers_to_scr_history_object>;
using devcommittee_withdrawals_to_scr_history_index
    = devcommittee_history_index<devcommittee_withdrawals_to_scr_history_object>;

} // namespace blockchain_history
} // namespace scorum

FC_REFLECT(scorum::blockchain_history::account_history_object, (id)(account)(sequence)(op))
FC_REFLECT(scorum::blockchain_history::account_transfers_to_scr_history_object, (id)(account)(sequence)(op))
FC_REFLECT(scorum::blockchain_history::account_transfers_to_sp_history_object, (id)(account)(sequence)(op))
FC_REFLECT(scorum::blockchain_history::account_withdrawals_to_scr_history_object, (id)(account)(sequence)(op)(progress))

FC_REFLECT(scorum::blockchain_history::devcommittee_history_object, (id)(op))
FC_REFLECT(scorum::blockchain_history::devcommittee_transfers_to_scr_history_object, (id)(op))
FC_REFLECT(scorum::blockchain_history::devcommittee_withdrawals_to_scr_history_object, (id)(op)(progress))

CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::account_history_object,
                         scorum::blockchain_history::account_operations_full_history_index)

CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::account_transfers_to_scr_history_object,
                         scorum::blockchain_history::account_transfers_to_scr_history_index)

CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::account_transfers_to_sp_history_object,
                         scorum::blockchain_history::account_transfers_to_sp_history_index)

CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::account_withdrawals_to_scr_history_object,
                         scorum::blockchain_history::account_withdrawals_to_scr_history_index)

CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::devcommittee_history_object,
                         scorum::blockchain_history::devcommittee_operations_full_history_index)

CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::devcommittee_transfers_to_scr_history_object,
                         scorum::blockchain_history::devcommittee_transfers_to_scr_history_index)

CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::devcommittee_withdrawals_to_scr_history_object,
                         scorum::blockchain_history::devcommittee_withdrawals_to_scr_history_index)
