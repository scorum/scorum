#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <vector>

namespace scorum {
namespace chain {

class budget_object;
class budget_schedule_object;
class account_object;

/** DB service for operations with budget objects:
 *  -------------------------------------------------
 *  ### budget_object, budget_schedule_object, budget_with_schedule_object. ###
 *
 *  This is the normalized data scheme:
 *
 *  ### For ordinary budget type: ###
 *      [budget_object] 1 <--
 *                           {
 *                           m
 *              [budget_with_schedule_object]
 *                           m
 *                           }--> 1 [budget_schedule_object]
 *
 *  ### For genesis budget: ###
 *      [budget_object] (no any schedule)
 *
 *      @warning For genesis budget distribution is happens like for budget
 *               with one \c unconditional schedule.
*/
class dbs_budget : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_budget(database& db);

public:
    typedef std::vector<budget_id_type> budget_ids_type;
    typedef std::vector<budget_schedule_id_type> budget_schedule_ids_type;

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
     * @returns genesis budget object
     */
    const budget_object& create_genesis_budget(const asset& balance_in_scorum, const share_type& per_block);
    /** Create budget.
     *  The owner has abilities for all operations (for update, close and schedule operations).
     *
     * @param owner the name of the owner
     * @param content_permlink the budget target identity (post or other)
     * @param balance_in_scorum the total balance (use SCORUM_SYMBOL)
     * @param per_block the amount for asset distribution
     *                  (the distribution is allowed one time per block)
     * @param auto_close the flag that control the behavior
     *                      in a situation when the budget becomes zero
     * @returns a budget object
     */
    const budget_object& create_budget(const account_object& owner,
                                       const optional<string>& content_permlink,
                                       const asset& balance_in_scorum,
                                       const share_type& per_block,
                                       bool auto_close = false /*owner manages live of the own budget*/
                                       );
    /** Close budget.
     *  Delete the budget and all associated objects,
     *  cash back from budget to owner account.
     *
     * @warning It is not allowed for genesis budget.
     *
     * @param budget the budget to close
     * @param content_permlink the budget target identity (post or other)
     * @param balance_in_scorum the total balance (use SCORUM_SYMBOL)
     * @param per_block the amount for asset distribution
     *                  (the distribution is allowed one time per block)
     * @param auto_close the flag that control the behavior
     *                      in a situation when the budget becomes zero
     */
    void close_budget(const budget_object& budget);
    /** Increase budget balance.
     *
     * @param budget the budget to modify
     * @param scorums balance to add (use SCORUM_SYMBOL)
     */
    void increase_balance(const budget_object& budget, const asset& scorums);
    /** Decrease budget balance.
     *
     * @param budget the budget to modify
     * @param scorums balance to subtract (use SCORUM_SYMBOL)
     */
    void decrease_balance(const budget_object& budget, const asset& scorums);
    /** Modify budget 'autoclose' behavior.
     *
     * @warning It is not allowed for genesis budget.
     * @warning If budget has zero balance and auto_close becames true,
     *          it is try to close budget.
     *
     * @param budget the budget to modify
     * @param auto_close the flag that control the behavior
     *                      in a situation when the budget becomes zero
     */
    void update_budget(const budget_object& budget, bool auto_close);
    /** Lists all schedules associated for budget.
     *
     * @warning It is not allowed for genesis budget.
     *
     * @param budget the budget to modify
     * @returns a list of schedules ids to get the single schedule (get_schedule)
     */
    budget_schedule_ids_type get_schedules(const budget_object& budget) const;
    /** To get the single schedule.
     *
     * @param budget the requested budget
     * @returns a schedule object
     */
    const budget_schedule_object& get_schedule(budget_schedule_id_type id) const;
    /** To get the single schedule by budget.
     *  Use if you know exactly that there is only one schedule for budget
     *  to simplify calling (you don' need in schedules list enumeration).
     *
     * @warning It is not allowed for genesis budget.
     *
     * @param id the schedule id
     * @returns a schedule object
     */
    const budget_schedule_object& get_any_schedule(const budget_object& budget) const;
    /** Create additional schedule.
     *  There is one schedule already (it is was created while budget creation was done),
     *  but to create complex schedule (with multiple items), you must create some more.
     *
     * @warning This function validates input schedule params.
     * @warning It is not allowed for genesis budget.
     *
     * @param budget the budget to modify
     * @param schedule_alg the schedule algorithm id or empty
     *           (use value from budget_schedule_object::budget_schedule_type or empty)
     * @param start_date the start date or empty
     * @param end_date the end date or empty
     * @param period the period or empty
     * @param now the now time or empty (for empty function obtains the now from database)
     */
    void append_schedule(const budget_object& budget,
                         const optional<uint16_t>& schedule_alg,
                         const optional<time_point_sec>& start_date,
                         const optional<time_point_sec>& end_date,
                         const optional<uint32_t>& period,
                         const optional<time_point_sec>& now = optional<time_point_sec>());
    /** Edit schedule.
     *
     * @warning This function validates input schedule params.
     * @warning It is not allowed for genesis budget.
     *
     * @param schedule the schedule to modify
     * @param schedule_alg the schedule algorithm id or empty
     *           (use value from budget_schedule_object::budget_schedule_type or empty)
     * @param start_date the start date or empty
     * @param end_date the end date or empty
     * @param period the period or empty
     * @param now the now time or empty (for empty function obtains the now from database)
     */
    void adjust_schedule(const budget_schedule_object& schedule,
                         const optional<uint16_t>& schedule_alg,
                         const optional<time_point_sec>& start_date,
                         const optional<time_point_sec>& end_date,
                         const optional<uint32_t>& period,
                         const optional<time_point_sec>& now = optional<time_point_sec>());
    /** Edit schedule.
     *  This tunction remove schedule from asset distribution.
     *  If the all schedules are locked no more asset will be distributed.
     *
     * @warning It is not allowed for genesis budget.
     *
     * @param schedule the schedule to modify
     */
    void lock_schedule(const budget_schedule_object& schedule);
    /** Edit schedule.
     *  This tunction restore schedule distribution (lock_schedule).
     *
     * @warning It is not allowed for genesis budget.
     *
     * @param schedule the schedule to modify
     */
    void unlock_schedule(const budget_schedule_object& schedule);
    /** Delete schedule and all associated objects.
     *
     * @warning If the only one schedule remains, that schedule will not be deleted.
     *          In this case remain schedule will be locked (lock_schedule)
     * @warning It is not allowed for genesis budget.
     *
     * @param schedule the schedule to remove (or lock)
     */
    void remove_schedule(const budget_schedule_object& schedule);
    /** Delete all schedules associated with budget.
     *
     * @warning If the only one schedule remains, that schedule will not be deleted.
     *          In this case remain schedule will be locked (lock_schedule)
     * @warning It is not allowed for genesis budget.
     *
     * @param budget the budget to modify
     */
    void crear_schedules(const budget_object& budget);
    /** Distribute asset from budget.
     *  This operation takes into account the schedules and last block number
     *
     * @param budget the budget that is distributed
     * @param now the now time or empty (for empty function obtains the now from database)
     */
    asset allocate_cash(const budget_object& budget, const optional<time_point_sec>& now = optional<time_point_sec>());

private:
    // return schedule_alg
    uint16_t _validate_schedule_input(const optional<uint16_t>& schedule_alg,
                                      const optional<time_point_sec>& start_date,
                                      const optional<time_point_sec>& end_date,
                                      const optional<uint32_t>& period);
    // without input validation
    void _adjust_schedule(const budget_schedule_object&,
                          uint16_t schedule_alg,
                          const optional<time_point_sec>& start_date,
                          const optional<time_point_sec>& end_date,
                          const optional<uint32_t>& period,
                          const optional<time_point_sec>& now);
    void _check_autoclose(const budget_object&);
};
}
}
