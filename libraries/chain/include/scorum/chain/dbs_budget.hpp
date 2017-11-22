#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <vector>

namespace scorum {
namespace chain {

class budget_object;
class budget_schedule_object;
class account_object;

// DB operations with budget_*** objects
//
class dbs_budget : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_budget(database& db);

public:

    typedef std::vector<budget_id_type> budget_ids_type;
    typedef std::vector<budget_schedule_id_type> budget_schedule_ids_type;

    budget_ids_type get_budgets(const account_name_type& owner) const;
    const budget_object &get_budget(budget_id_type id) const;
    const budget_object &get_any_budget(const account_name_type& owner) const;

    const budget_object &create_genesis_budget(const asset &balance_in_scorum,
                               const share_type &per_block,
                               bool auto_close = true);

    const budget_object &create_budget(const account_object &owner,
                       const optional<string> &content_permlink,
                       const asset &balance_in_scorum,
                       const share_type &per_block,
                       bool auto_close = false /*owner manages live of the own budget*/
                       );

    void close_budget(const budget_object &);

    void increase_balance(const budget_object& budget, const asset& scorums);
    void decrease_balance(const budget_object& budget, const asset& scorums);

    void update_budget(const budget_object& budget, bool auto_close);

    budget_schedule_ids_type get_budget_schedules(const budget_object& budget) const;
    const budget_schedule_object& get_budget_schedule(budget_schedule_id_type id) const;

    void adjust_budget_schedule(const budget_schedule_object&,
                                uint16_t schedule_alg,
                                const optional<time_point_sec>& start_date,
                                const optional<time_point_sec>& end_date,
                                const optional<time_point_sec>& period);

    asset allocate_cash(const budget_object&, const optional<time_point_sec>& now = optional<time_point_sec>());

};

}
}
