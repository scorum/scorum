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

class fund_budget_object : public object<fund_budget_object_type, fund_budget_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(fund_budget_object)

    typedef typename object<fund_budget_object_type, fund_budget_object>::id_type id_type;

    id_type id;

    time_point_sec created = time_point_sec::min();
    time_point_sec start = time_point_sec::min();
    time_point_sec deadline = time_point_sec::maximum();

    asset balance = asset(0, SP_SYMBOL);
    asset per_block = asset(0, SP_SYMBOL);
};

template <budget_type> struct budget_traits;

template <> struct budget_traits<budget_type::banner>
{
    static constexpr uint16_t object_type_v = banner_budget_object_type;
};

template <> struct budget_traits<budget_type::post>
{
    static constexpr uint16_t object_type_v = post_budget_object_type;
};

/// @addtogroup adv_api
/// @{
template <budget_type budget_type_v>
class adv_budget_object : public object<budget_traits<budget_type_v>::object_type_v, adv_budget_object<budget_type_v>>
{
public:
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(adv_budget_object, (json_metadata))

    using id_type = oid<adv_budget_object<budget_type_v>>;

    id_type id;

    account_name_type owner;
    fc::shared_string json_metadata;

    time_point_sec created = time_point_sec::min();
    time_point_sec start = time_point_sec::min();
    time_point_sec deadline = time_point_sec::maximum();

    asset balance = asset(0, SCORUM_SYMBOL);
    asset per_block = asset(0, SCORUM_SYMBOL);

    fc::time_point_sec cashout_time = time_point_sec::min();
    asset owner_pending_income = asset(0, SCORUM_SYMBOL);
    asset budget_pending_outgo = asset(0, SCORUM_SYMBOL);

    bool is_positive_balance() const
    {
        return balance.amount != 0;
    }
};
/// @}

struct by_owner_name;
struct by_per_block;

template <typename TBudgetObject> using id_t = typename TBudgetObject::id_type;

// clang-format off
using fund_budget_index
    = shared_multi_index_container<fund_budget_object,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<fund_budget_object,
                                                                    fund_budget_object::id_type,
                                                                    &fund_budget_object::id>>>>;

struct by_cashout_time;
struct by_balances;

template <budget_type budget_type_v>
using adv_budget_index
    = shared_multi_index_container<adv_budget_object<budget_type_v>,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<adv_budget_object<budget_type_v>,
                                                                    id_t<adv_budget_object<budget_type_v>>,
                                                                    &adv_budget_object<budget_type_v>::id>>,
                                              ordered_non_unique<tag<by_owner_name>,
                                                                 member<adv_budget_object<budget_type_v>,
                                                                        account_name_type,
                                                                        &adv_budget_object<budget_type_v>::owner>>,
                                              ordered_non_unique<tag<by_balances>,
                                                                 composite_key<adv_budget_object<budget_type_v>,
                                                                               member<adv_budget_object<budget_type_v>,
                                                                                      asset,
                                                                                      &adv_budget_object<budget_type_v>::balance>,
                                                                               member<adv_budget_object<budget_type_v>,
                                                                                      asset,
                                                                                      &adv_budget_object<budget_type_v>::owner_pending_income>,
                                                                               member<adv_budget_object<budget_type_v>,
                                                                                      asset,
                                                                                      &adv_budget_object<budget_type_v>::budget_pending_outgo>>>,
                                              ordered_non_unique<tag<by_cashout_time>,
                                                                 member<adv_budget_object<budget_type_v>,
                                                                        fc::time_point_sec,
                                                                        &adv_budget_object<budget_type_v>::cashout_time>>,
                                              ordered_unique<tag<by_per_block>,
                                                             composite_key<adv_budget_object<budget_type_v>,
                                                                           const_mem_fun<adv_budget_object<budget_type_v>,
                                                                                         bool,
                                                                                         &adv_budget_object<budget_type_v>::is_positive_balance>,
                                                                           member<adv_budget_object<budget_type_v>,
                                                                                  asset,
                                                                                  &adv_budget_object<budget_type_v>::per_block>,
                                                                           member<adv_budget_object<budget_type_v>,
                                                                                  id_t<adv_budget_object<budget_type_v>>,
                                                                                  &adv_budget_object<budget_type_v>::id>>,
                                                             composite_key_compare<std::greater<bool>,
                                                                                   std::greater<asset>,
                                                                                   std::less<id_t<adv_budget_object<budget_type_v>>>>>>>;
// clang-format on

using post_budget_object = adv_budget_object<budget_type::post>;
using banner_budget_object = adv_budget_object<budget_type::banner>;

using post_budget_index = adv_budget_index<budget_type::post>;
using banner_budget_index = adv_budget_index<budget_type::banner>;

} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT(scorum::chain::fund_budget_object, (id)(created)(start)(deadline)(balance)(per_block))
FC_REFLECT(scorum::chain::post_budget_object,
           (id)
           (owner)
           (json_metadata)
           (created)
           (start)
           (deadline)
           (balance)
           (per_block)
           (cashout_time)
           (owner_pending_income)
           (budget_pending_outgo))
FC_REFLECT(scorum::chain::banner_budget_object,
           (id)
           (owner)
           (json_metadata)
           (created)
           (start)
           (deadline)
           (balance)
           (per_block)
           (cashout_time)
           (owner_pending_income)
           (budget_pending_outgo))
// clang-format on

CHAINBASE_SET_INDEX_TYPE(scorum::chain::fund_budget_object, scorum::chain::fund_budget_index)
CHAINBASE_SET_INDEX_TYPE(scorum::chain::post_budget_object, scorum::chain::post_budget_index)
CHAINBASE_SET_INDEX_TYPE(scorum::chain::banner_budget_object, scorum::chain::banner_budget_index)
