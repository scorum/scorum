#include <scorum/chain/services/budgets.hpp>
#include <algorithm>

#include <scorum/chain/schema/budget_objects.hpp>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/utils/math.hpp>

namespace scorum {
namespace chain {
namespace detail {
asset fund_calculate_per_block(time_point_sec start_date, time_point_sec end_date, const asset& balance)
{
    FC_ASSERT(start_date < end_date, "Start time ${1} must be less end time ${2}", ("1", start_date)("2", end_date));

    auto per_block = balance;

    uint32_t delta_in_sec = end_date.sec_since_epoch() - start_date.sec_since_epoch();

    per_block *= SCORUM_BLOCK_INTERVAL;
    per_block /= delta_in_sec;

    // non zero budget must return at least one satoshi
    per_block.amount = std::max(per_block.amount, share_type(1));

    return per_block;
}

asset adv_calculate_per_block(time_point_sec start,
                              time_point_sec deadline,
                              time_point_sec last_block_time,
                              const asset& balance)
{
    FC_ASSERT(start <= deadline, "Start time ${1} must be less or equal end time ${2}", ("1", start)("2", deadline));

    auto aligned_start_sec
        = utils::ceil(start.sec_since_epoch(), last_block_time.sec_since_epoch(), SCORUM_BLOCK_INTERVAL);

    auto aligned_deadline_sec
        = utils::ceil(deadline.sec_since_epoch(), last_block_time.sec_since_epoch(), SCORUM_BLOCK_INTERVAL);

    auto per_blocks_count = 1 + ((aligned_deadline_sec - aligned_start_sec) / SCORUM_BLOCK_INTERVAL);
    auto per_block = balance / per_blocks_count;

    // non zero budget must return at least one satoshi
    per_block.amount = std::max(per_block.amount, share_type(1));

    return per_block;
}
}

dbs_fund_budget::dbs_fund_budget(database& db)
    : dbs_service_base<fund_budget_service_i>(db)
    , _dprops_svc(db.dynamic_global_property_service())
{
}

const fund_budget_object& dbs_fund_budget::create_budget(const asset& balance, time_point_sec start, time_point_sec end)
{
    try
    {
        FC_ASSERT(balance.symbol() == SP_SYMBOL, "Invalid asset type (symbol).");
        FC_ASSERT(start < end, "Invalid dates.");

        auto per_block = detail::fund_calculate_per_block(start, end, balance);

        return create([&](fund_budget_object& budget) {
            budget.created = _dprops_svc.head_block_time();
            budget.start = start;
            budget.deadline = end;
            budget.balance = balance;
            budget.per_block = per_block;
        });
    }
    FC_CAPTURE_AND_RETHROW((balance)(start)(end))
}

asset dbs_fund_budget::allocate_cash(const fund_budget_object& budget)
{
    auto allocated = std::min(budget.balance, budget.per_block);

    update(budget, [&](fund_budget_object& b) { b.balance -= allocated; });
    if (budget.balance.amount <= 0)
        remove(budget);

    return allocated;
}

template <budget_type budget_type_v>
dbs_advertising_budget<budget_type_v>::dbs_advertising_budget(database& db)
    : dbs_service_base<typename budget_service_traits<budget_type_v>::service_type>(db)
    , _dgp_svc(db.dynamic_global_property_service())
    , _account_svc(db.account_service())
{
}

template <budget_type budget_type_v>
const adv_budget_object<budget_type_v>&
dbs_advertising_budget<budget_type_v>::create_budget(const account_name_type& owner,
                                                     const asset& balance,
                                                     time_point_sec start,
                                                     time_point_sec end,
                                                     const std::string& json_metadata)
{
    try
    {
        FC_ASSERT(balance.symbol() == SCORUM_SYMBOL, "Invalid asset type (symbol).");
        FC_ASSERT(balance.amount > 0, "Invalid balance.");

        auto head_block_time = _dgp_svc.head_block_time();
        auto per_block = detail::adv_calculate_per_block(start, end, head_block_time, balance);

        auto aligned_start_sec
            = utils::ceil(start.sec_since_epoch(), head_block_time.sec_since_epoch(), SCORUM_BLOCK_INTERVAL);
        auto cashout_sec = aligned_start_sec + SCORUM_ADVERTISING_CASHOUT_PERIOD_SEC - SCORUM_BLOCK_INTERVAL;

        const auto& budget = this->create([&](adv_budget_object<budget_type_v>& budget) {
            budget.owner = owner;
#ifndef IS_LOW_MEM
            fc::from_string(budget.json_metadata, json_metadata);
#endif
            budget.created = head_block_time;
            budget.cashout_time = fc::time_point_sec(cashout_sec);
            budget.start = start;
            budget.deadline = end;
            budget.balance = balance;
            budget.per_block = per_block;
        });

        _account_svc.decrease_balance(_account_svc.get_account(owner), balance);

        update_totals([&](adv_total_stats::budget_type_stat& statistic) { statistic.volume += balance; });

        return budget;
    }
    FC_CAPTURE_AND_RETHROW((owner)(balance)(start)(end)(json_metadata))
}

template <budget_type budget_type_v>
const adv_budget_object<budget_type_v>&
dbs_advertising_budget<budget_type_v>::get(const oid<adv_budget_object<budget_type_v>>& id) const
{
    try
    {
        return this->get_by(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

template <budget_type budget_type_v>
bool dbs_advertising_budget<budget_type_v>::is_exists(const oid<adv_budget_object<budget_type_v>>& id) const
{
    try
    {
        return nullptr != this->find_by(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

template <budget_type budget_type_v>
const adv_budget_object<budget_type_v>*
dbs_advertising_budget<budget_type_v>::find(const oid<adv_budget_object<budget_type_v>>& id) const
{
    try
    {
        return this->find_by(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

template <budget_type budget_type_v>
typename dbs_advertising_budget<budget_type_v>::budgets_type dbs_advertising_budget<budget_type_v>::get_budgets() const
{
    try
    {
        return this->template get_range_by<by_id>(::boost::multi_index::unbounded, ::boost::multi_index::unbounded);
    }
    FC_CAPTURE_AND_RETHROW(())
}

template <budget_type budget_type_v>
typename dbs_advertising_budget<budget_type_v>::budgets_type
dbs_advertising_budget<budget_type_v>::get_top_budgets(const fc::time_point_sec& until, uint16_t limit) const
{
    namespace ba = boost::adaptors;
    try
    {
        budgets_type result;
        result.reserve(limit);

        // TODO: will be refactored using db_accessors
        auto& idx = this->db_impl().template get_index<adv_budget_index<budget_type_v>, by_per_block>();
        auto from = idx.begin();
        auto to = idx.lower_bound(false); // including

        auto rng = boost::make_iterator_range(from, to)
            | ba::filtered([&](const adv_budget_object<budget_type_v>& obj) { return obj.start <= until; });

        // TODO: will be refactored using take_n range adaptor
        for (auto it = rng.begin(); limit && it != rng.end(); ++it, --limit)
        {
            result.push_back(std::cref(*it));
        }
        return result;
    }
    FC_CAPTURE_AND_RETHROW((until)(limit))
}

template <budget_type budget_type_v>
typename dbs_advertising_budget<budget_type_v>::budgets_type
dbs_advertising_budget<budget_type_v>::get_top_budgets(const fc::time_point_sec& until) const
{
    try
    {
        return this->get_top_budgets(until, -1);
    }
    FC_CAPTURE_AND_RETHROW((until))
}

template <budget_type budget_type_v>
std::set<std::string>
dbs_advertising_budget<budget_type_v>::lookup_budget_owners(const std::string& lower_bound_owner_name,
                                                            uint32_t limit) const
{
    try
    {
        std::set<std::string> result;

        const auto& budgets_by_owner_name
            = this->db_impl().template get_index<adv_budget_index<budget_type_v>, by_owner_name>();

        // prepare output if limit > 0
        for (auto itr = budgets_by_owner_name.lower_bound(lower_bound_owner_name);
             limit && itr != budgets_by_owner_name.end(); ++itr)
        {
            --limit;
            result.insert(itr->owner);
        }
        return result;
    }
    FC_CAPTURE_AND_RETHROW((lower_bound_owner_name)(limit))
}

template <budget_type budget_type_v>
typename dbs_advertising_budget<budget_type_v>::budgets_type
dbs_advertising_budget<budget_type_v>::get_budgets(const account_name_type& owner) const
{
    try
    {
        return this->template get_range_by<by_owner_name>(owner <= ::boost::lambda::_1, ::boost::lambda::_1 <= owner);
    }
    FC_CAPTURE_AND_RETHROW((owner))
}

template <budget_type budget_type_v>
typename dbs_advertising_budget<budget_type_v>::budgets_type
dbs_advertising_budget<budget_type_v>::get_pending_budgets() const
{
    try
    {
        auto head_time = _dgp_svc.head_block_time();
        return this->template get_range_by<by_cashout_time>(boost::multi_index::unbounded,
                                                            ::boost::lambda::_1 <= head_time);
    }
    FC_CAPTURE_AND_RETHROW()
}

template <budget_type budget_type_v>
asset dbs_advertising_budget<budget_type_v>::allocate_cash(const adv_budget_object<budget_type_v>& budget)
{
    this->update(budget, [&](adv_budget_object<budget_type_v>& b) { b.balance -= budget.per_block; });

    update_totals([&](adv_total_stats::budget_type_stat& statistic) { statistic.volume -= budget.per_block; });

    if (budget.deadline <= _dgp_svc.head_block_time())
        finish_budget(budget.id);

    return budget.per_block;
}

template <budget_type budget_type_v>
void dbs_advertising_budget<budget_type_v>::update_pending_payouts(const adv_budget_object<budget_type_v>& budget,
                                                                   const asset& owner_incoming,
                                                                   const asset& budget_outgoing)
{
    this->update(budget, [&](adv_budget_object<budget_type_v>& b) {
        b.owner_pending_income += owner_incoming;
        b.budget_pending_outgo += budget_outgoing;
    });

    update_totals([&](adv_total_stats::budget_type_stat& statistic) {
        statistic.owner_pending_income += owner_incoming;
        statistic.budget_pending_outgo += budget_outgoing;
    });
}

template <budget_type budget_type_v>
asset dbs_advertising_budget<budget_type_v>::perform_pending_payouts(const budgets_type& budgets)
{
    auto budgets_outgo = asset(0, SCORUM_SYMBOL);
    for (const adv_budget_object<budget_type_v>& budget : budgets)
    {
        budgets_outgo += budget.budget_pending_outgo;
        _account_svc.increase_balance(_account_svc.get_account(budget.owner), budget.owner_pending_income);

        update_totals([&](adv_total_stats::budget_type_stat& statistic) {
            statistic.owner_pending_income -= budget.owner_pending_income;
            statistic.budget_pending_outgo -= budget.budget_pending_outgo;
        });

        this->update(budget, [&](adv_budget_object<budget_type_v>& b) {
            b.owner_pending_income.amount = 0;
            b.budget_pending_outgo.amount = 0;
            b.cashout_time = _dgp_svc.head_block_time() + SCORUM_ADVERTISING_CASHOUT_PERIOD_SEC;
        });
    }

    return budgets_outgo;
}

template <budget_type budget_type_v>
void dbs_advertising_budget<budget_type_v>::finish_budget(const oid<adv_budget_object<budget_type_v>>& id)
{
    const auto& budget = get(id);

    update_totals([&](adv_total_stats::budget_type_stat& statistic) {
        statistic.owner_pending_income += budget.balance;
        statistic.volume -= budget.balance;
    });

    this->update(budget, [&](adv_budget_object<budget_type_v>& b) {
        b.cashout_time = _dgp_svc.head_block_time();
        b.owner_pending_income += b.balance;
        b.balance.amount = 0;
    });
}

template <budget_type budget_type_v>
typename dbs_advertising_budget<budget_type_v>::budgets_type
dbs_advertising_budget<budget_type_v>::get_empty_budgets() const
{
    auto zero = asset(0, SCORUM_SYMBOL);
    auto empty_budgets = this->template get_range_by<by_balances>(
        boost::multi_index::unbounded, ::boost::lambda::_1 <= std::make_tuple(zero, zero, zero));

    return empty_budgets;
}

template <budget_type budget_type_v>
void dbs_advertising_budget<budget_type_v>::update_totals(
    std::function<void(adv_total_stats::budget_type_stat&)> callback)
{
    if (budget_type_v == budget_type::banner)
    {
        _dgp_svc.update([&](dynamic_global_property_object& dgp) { callback(dgp.advertising.banner_budgets); });
    }
    else if (budget_type_v == budget_type::post)
    {
        _dgp_svc.update([&](dynamic_global_property_object& dgp) { callback(dgp.advertising.post_budgets); });
    }
    else
    {
        FC_THROW("unsuported budget type");
    }
}

template class dbs_advertising_budget<budget_type::post>;
template class dbs_advertising_budget<budget_type::banner>;
}
}
