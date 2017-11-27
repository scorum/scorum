#include <scorum/chain/dbs_budget.hpp>
#include <scorum/chain/database.hpp>
#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/account_object.hpp>

#include <scorum/chain/budget_objects.hpp>
#include <scorum/chain/database.hpp>

#include <tuple>

namespace scorum {
namespace chain {

dbs_budget::dbs_budget(database& db)
    : _base_type(db)
{
    db_impl().get_index<budget_index>().indicies().get<by_owner_name>();
}

dbs_budget::budget_ids_type dbs_budget::get_budgets(const account_name_type& owner) const
{
    budget_ids_type ret;

    auto it_pair = db_impl().get_index<budget_index>().indicies().get<by_owner_name>().equal_range(owner);
    auto it = it_pair.first;
    auto it_end = it_pair.second;
    FC_ASSERT(it != it_end, "budget not found");
    while (it != it_end)
    {
        ret.push_back(it->id);
        it++;
    }

    return ret;
}

const budget_object& dbs_budget::get_budget(budget_id_type id) const
{
    try
    {
        return db_impl().get<budget_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW() // to write __FILE__, __LINE__ from here
}

const budget_object& dbs_budget::get_any_budget(const account_name_type& owner) const
{
    auto it_pair = db_impl().get_index<budget_index>().indicies().get<by_owner_name>().equal_range(owner);
    auto it_begin = it_pair.first;
    auto it_end = it_pair.second;
    FC_ASSERT(it_begin != it_end, "budget not found");

    return *it_begin;
}

const budget_object& dbs_budget::create_fund_budget(const asset& balance_in_scorum,
                                                    const share_type& per_block,
                                                    const time_point_sec& deadline)
{
    FC_ASSERT(balance_in_scorum.symbol == SCORUM_SYMBOL, "invalid asset type (symbol)");
    FC_ASSERT(balance_in_scorum.amount > 0, "invalid balance_in_scorum");
    FC_ASSERT(per_block > 0, "invalid per_block");

    auto head_block_num = db_impl().head_block_num();

    const budget_object& new_budget = db_impl().create<budget_object>([&](budget_object& budget) {
        budget.owner = SCORUM_ROOT_POST_PARENT;
        budget.balance = balance_in_scorum;
        budget.per_block = per_block;
        // allocate cash only after next block generation
        budget.last_allocated_block = head_block_num;
    });

    // there are not schedules for genesis budget
    return new_budget;
}

const budget_object& dbs_budget::create_budget(const account_object& owner,
                                               const optional<string>& content_permlink,
                                               const asset& balance_in_scorum,
                                               const share_type& per_block,
                                               const time_point_sec& deadline)
{
    FC_ASSERT(owner.name != SCORUM_ROOT_POST_PARENT, "not allowed for ordinary budget");
    FC_ASSERT(balance_in_scorum.symbol == SCORUM_SYMBOL, "invalid asset type (symbol)");
    FC_ASSERT(balance_in_scorum.amount > 0, "invalid balance_in_scorum");
    FC_ASSERT(per_block > 0, "invalid per_block");

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();

    dbs_account& account_service = db().obtain_service<dbs_account>();

    FC_ASSERT(owner.balance >= balance_in_scorum, "insufficient funds");

    account_service.decrease_balance(owner, balance_in_scorum);

    auto head_block_num = db_impl().head_block_num();

    const budget_object& new_budget = db_impl().create<budget_object>([&](budget_object& budget) {
        budget.created = props.time;
        budget.owner = owner.name;
        budget.balance = balance_in_scorum;
        budget.per_block = per_block;
        // allocate cash only after next block generation
        budget.last_allocated_block = head_block_num;
    });

    return new_budget;
}

void dbs_budget::close_budget(const budget_object& budget)
{
    FC_ASSERT(budget.owner != SCORUM_ROOT_POST_PARENT, "not allowed for genesis budget");

    const budget_object& actual_budget = get_budget(budget.id);

    dbs_account& account_service = db().obtain_service<dbs_account>();

    const auto& owner = account_service.get_account(actual_budget.owner);

    // withdraw all balance rest asset back to owner
    //
    asset repayable = actual_budget.balance;
    if (repayable.amount > 0)
    {
        // check if input budget state != budget in DB
        repayable = _decrease_balance(actual_budget, repayable);
        account_service.increase_balance(owner, repayable);
    }

    // delete budget
    //
    db_impl().remove(actual_budget);
}

asset dbs_budget::allocate_cash(const budget_object& budget, const optional<time_point_sec>& now)
{
    asset ret(0, SCORUM_SYMBOL);

    const budget_object& actual_budget = get_budget(budget.id);

    dbs_budget::_time t = _get_now(now);
    auto head_block_num = db_impl().head_block_num();

    if (actual_budget.last_allocated_block >= head_block_num)
    {
        return ret; // empty (allocation waits new block)
    }

    FC_ASSERT(actual_budget.per_block > 0, "invalid per_block");
    ret = _decrease_balance(actual_budget, asset(actual_budget.per_block, SCORUM_SYMBOL));

    if (actual_budget.deadline <= t)
    {
        close_budget(actual_budget);
    }
    else
    {
        if (!_check_autoclose(actual_budget))
        {
            db_impl().modify(actual_budget, [&](budget_object& b) { b.last_allocated_block = head_block_num; });
        }
    }
    return ret;
}

asset dbs_budget::_decrease_balance(const budget_object& budget, const asset& balance_in_scorum)
{
    FC_ASSERT(balance_in_scorum.symbol == SCORUM_SYMBOL, "invalid asset type (symbol)");
    FC_ASSERT(balance_in_scorum.amount > 0, "invalid balance_in_scorum");

    asset ret(0, SCORUM_SYMBOL);

    db_impl().modify(budget, [&](budget_object& b) {
        if (b.balance.amount > 0 && balance_in_scorum.amount <= b.balance.amount)
        {
            b.balance -= balance_in_scorum;
            ret = balance_in_scorum;
        }
        else
        {
            ret = b.balance;
            b.balance.amount = 0;
        }
    });

    return ret;
}

bool dbs_budget::_check_autoclose(const budget_object& budget)
{
    if (budget.balance.amount <= 0)
    {
        close_budget(budget);
        return true;
    }
    else
    {
        return false;
    }
}
}
}
