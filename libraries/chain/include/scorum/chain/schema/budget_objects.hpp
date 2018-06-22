#pragma once

#include <fc/fixed_string.hpp>
#include <fc/static_variant.hpp>
#include <fc/shared_containers.hpp>

#include <scorum/protocol/authority.hpp>
#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

template <uint16_t ObjectType, asset_symbol_type SymbolType>
class budget_object : public object<ObjectType, budget_object<ObjectType, SymbolType>>
{
public:
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(budget_object, (permlink))

    typedef typename object<ObjectType, budget_object<ObjectType, SymbolType>>::id_type id_type;

    id_type id;

    account_name_type owner;
    fc::shared_string permlink;

    time_point_sec created = time_point_sec::min();
    time_point_sec start = time_point_sec::min();
    time_point_sec deadline = time_point_sec::maximum();

    asset balance = asset(0, SymbolType);
    asset per_block = asset(0, SymbolType);

    uint32_t last_cashout_block = 0;
};

struct by_owner_name;
struct by_start_time;

template <typename BudgetObjectType>
using budget_index
    = shared_multi_index_container<BudgetObjectType,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<BudgetObjectType,
                                                                    typename BudgetObjectType::id_type,
                                                                    &BudgetObjectType::id>>,
                                              ordered_non_unique<tag<by_owner_name>,
                                                                 member<BudgetObjectType,
                                                                        account_name_type,
                                                                        &BudgetObjectType::owner>>,
                                              ordered_unique<tag<by_start_time>,
                                                             composite_key<BudgetObjectType,
                                                                           member<BudgetObjectType,
                                                                                  time_point_sec,
                                                                                  &BudgetObjectType::start>,
                                                                           member<BudgetObjectType,
                                                                                  typename BudgetObjectType::id_type,
                                                                                  &BudgetObjectType::id>>>>>;

using fund_budget_object = budget_object<fund_budget_object_type, SP_SYMBOL>;
using post_budget_object = budget_object<post_budget_object_type, SCORUM_SYMBOL>;
using banner_budget_object = budget_object<banner_budget_object_type, SCORUM_SYMBOL>;

using fund_budget_index = budget_index<fund_budget_object>;
using post_budget_index = budget_index<post_budget_object>;
using banner_budget_index = budget_index<banner_budget_object>;

} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::fund_budget_object,
           (id)(owner)(permlink)(created)(start)(deadline)(balance)(per_block)(last_cashout_block))
FC_REFLECT(scorum::chain::post_budget_object,
           (id)(owner)(permlink)(created)(start)(deadline)(balance)(per_block)(last_cashout_block))
FC_REFLECT(scorum::chain::banner_budget_object,
           (id)(owner)(permlink)(created)(start)(deadline)(balance)(per_block)(last_cashout_block))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::fund_budget_object, scorum::chain::fund_budget_index)
CHAINBASE_SET_INDEX_TYPE(scorum::chain::post_budget_object, scorum::chain::post_budget_index)
CHAINBASE_SET_INDEX_TYPE(scorum::chain::banner_budget_object, scorum::chain::banner_budget_index)
