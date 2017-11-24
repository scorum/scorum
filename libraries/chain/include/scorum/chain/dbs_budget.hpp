#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <vector>

namespace scorum {
namespace chain {

class budget_object;
class budget_schedule_object;
class account_object;

/** DB service for operations with budget_object
 *  --------------------------------------------
*/
class dbs_budget : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_budget(database& db);

public:
    using budget_ids_type = std::vector<budget_id_type>;

    /** Lists all budgets registered for owner.
     *
     * @param owner the name of the owner
     *                   (use \c SCORUM_ROOT_POST_PARENT to get genesis budget)
     * @returns a list of budgets ids to get the single budget (et_budget)
     */
    budget_ids_type get_budgets(const account_name_type& owner) const;

    /** To get the single budget.
     *
     * @param id the budget id
     * @returns a budget object
     */
    const budget_object& get_budget(budget_id_type id) const;

    /** To get the single budget by owner.
     *  Use if you know exactly that there is only one budget for owner
     *  to simplify calling (you don' need in budgets list enumeration)
     *
     * @param owner the name of the owner
     *                   (use \c SCORUM_ROOT_POST_PARENT to get genesis budget)
     * @returns a budget object
     */
    const budget_object& get_any_budget(const account_name_type& owner) const;

    /** Create genesis budget.
     *
     *  @warning The genesis budget has no owner (to be precise, it has fake owner \c SCORUM_ROOT_POST_PARENT).
     *  @warning The genesis budget has some restrictions (for update, close and schedule operations).
     *  @warning There are not schedules for genesis budget (it is analogous to one unconditional schedule).
     *
     * @param balance_in_scorum the total balance (use SCORUM_SYMBOL)
     * @param per_block the amount for asset distribution
     *                  (the distribution is allowed one time per block)
     * @param deadline the deadline time to close budget (even if there is rest of balance)
     * @returns genesis budget object
     */
    const budget_object& create_genesis_budget(const asset& balance_in_scorum,
                                               const share_type& per_block,
                                               const time_point_sec& deadline);

    /** Create budget.
     *  The owner has abilities for all operations (for update, close and schedule operations).
     *
     * @param owner the name of the owner
     * @param content_permlink the budget target identity (post or other)
     * @param balance_in_scorum the total balance (use SCORUM_SYMBOL)
     * @param per_block the amount for asset distribution
     *                  (the distribution is allowed one time per block)
     * @param deadline the deadline time to close budget (even if there is rest of balance)
     * @returns a budget object
     */
    const budget_object& create_budget(const account_object& owner,
                                       const optional<string>& content_permlink,
                                       const asset& balance_in_scorum,
                                       const share_type& per_block,
                                       const time_point_sec& deadline
                                       );

    /** Close budget.
     *  Delete the budget, cash back from budget to owner account.
     *
     * @warning It is not allowed for genesis budget.
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

    asset _decrease_balance(const budget_object&, const asset& balance_in_scorum);
    bool _check_autoclose(const budget_object&);
};
}
}
