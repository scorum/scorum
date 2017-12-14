#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <scorum/chain/budget_objects.hpp>

namespace scorum {
namespace chain {

using std::string;

/** DB service for operations with budget_object
 *  --------------------------------------------
 */
class dbs_budget : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_budget() = delete;

protected:
    explicit dbs_budget(database& db);

public:
    using budget_refs_type = std::vector<std::reference_wrapper<const budget_object>>;

    /** Lists all budget owners.
     *
     *  @warning limit must be less or equal than SCORUM_LIMIT_BUDGETS_LIST_SIZE.
     *
     */
    std::set<string> lookup_budget_owners(const string& lower_bound_owner_name, uint32_t limit) const;

    /** Lists all budgets.
     *
     * @returns a list of budget objects
     */
    budget_refs_type get_budgets() const;

    /** Lists all budgets registered for owner.
     *
     * @param owner the name of the owner
     * @returns a list of budget objects
     */
    budget_refs_type get_budgets(const account_name_type& owner) const;

    /** Lists all fund budgets
     */
    const budget_object& get_fund_budget() const;

    /** Get budget by id
     */
    const budget_object& get_budget(budget_id_type id) const;

    /** Create fund budget (non any owner).
     *
     * @warning count of fund budgets must be less or equal than SCORUM_LIMIT_BUDGETS_PER_OWNER.
     *
     * @param balance the total balance (use SCORUM_SYMBOL)
     * @param deadline the deadline time to close budget (even if there is rest of balance)
     * @returns fund budget object
     */
    const budget_object& create_fund_budget(const asset& balance, const time_point_sec& deadline);

    /** Create budget.
     *  The owner has abilities for all operations (for update, close and schedule operations).
     *
     * @param owner the name of the owner
     * @param balance the total balance (use SCORUM_SYMBOL)
     * @param deadline the deadline time to close budget (even if there is rest of balance)
     * @param content_permlink the budget target identity (post or other)
     * @returns a budget object
     */
    const budget_object& create_budget(const account_object& owner,
                                       const asset& balance,
                                       const time_point_sec& deadline,
                                       const optional<string>& content_permlink = optional<string>());

    /** Close budget.
     *  Delete the budget, cash back from budget to owner account.
     *
     * @warning It is not allowed for fund budget.
     *
     * @param budget the budget to close
     */
    void close_budget(const budget_object& budget);

    /** Distribute asset from budget.
     *  This operation takes into account the deadline and last block number
     *
     * @param budget the budget that is distributed
     * @param now the now time or empty (for empty function obtains the now from database)
     */
    asset allocate_cash(const budget_object& budget, const optional<time_point_sec>& now = optional<time_point_sec>());

private:
    share_type
    _calculate_per_block(const time_point_sec& start_date, const time_point_sec& end_date, share_type balance_amount);
    asset _decrease_balance(const budget_object&, const asset& balance);
    bool _check_autoclose(const budget_object&);
    bool _is_fund_budget(const budget_object&) const;
    void _close_budget(const budget_object&);
    void _close_owned_budget(const budget_object&);
    void _close_fund_budget(const budget_object&);
    uint64_t _get_budget_count(const account_name_type& owner) const;
};
} // namespace chain
} // namespace scorum
