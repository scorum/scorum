#include <scorum/chain/dbs_budget.hpp>
#include <scorum/chain/database.hpp>
#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/account_object.hpp>

#include <tuple>

namespace scorum {
namespace chain {

dbs_budget::dbs_budget(database& db)
    : _base_type(db)
{
}

dbs_budget::budget_refs_type dbs_budget::get_budgets() const
{
    budget_refs_type ret;

    auto idx = db_impl().get_index<budget_index>().indicies();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    FC_ASSERT(it != it_end, "non any budget");
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        it++;
    }

    return ret;
}

uint64_t dbs_budget::get_budget_count() const
{
    return db_impl().get_index<budget_index>().indicies().size();
}

std::set<string> dbs_budget::lookup_budget_owners(const string& lower_bound_owner_name, uint32_t limit) const
{
    FC_ASSERT(limit <= SCORUM_BUDGET_LIMIT_DB_LIST_SIZE, "limit must be less or equal than ${1}",
              ("1", SCORUM_BUDGET_LIMIT_DB_LIST_SIZE));
    const auto& budgets_by_owner_name = db_impl().get_index<budget_index>().indices().get<by_owner_name>();
    set<string> result;

    for (auto itr = budgets_by_owner_name.lower_bound(lower_bound_owner_name);
         limit-- && itr != budgets_by_owner_name.end(); ++itr)
    {
        result.insert(itr->owner);
    }

    return result;
}

dbs_budget::budget_refs_type dbs_budget::get_budgets(const account_name_type& owner) const
{
    budget_refs_type ret;

    auto it_pair = db_impl().get_index<budget_index>().indicies().get<by_owner_name>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    FC_ASSERT(it != it_end, "budget not found");
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        it++;
    }

    return ret;
}

uint64_t dbs_budget::get_budget_count(const account_name_type& owner) const
{
    return db_impl().get_index<budget_index>().indicies().get<by_owner_name>().count(owner);
}

dbs_budget::budget_refs_type dbs_budget::get_fund_budgets() const
{
    return get_budgets(SCORUM_ROOT_POST_PARENT);
}

uint64_t dbs_budget::get_fund_budget_count() const
{
    return get_budget_count(SCORUM_ROOT_POST_PARENT);
}

const budget_object& dbs_budget::get_budget(budget_id_type id) const
{
    return db_impl().get<budget_object>(id);
}

const budget_object& dbs_budget::create_fund_budget(const asset& balance_in_scorum, const time_point_sec& deadline)
{
    FC_ASSERT(balance_in_scorum.symbol == SCORUM_SYMBOL, "invalid asset type (symbol)");
    FC_ASSERT(balance_in_scorum.amount > 0, "invalid balance_in_scorum");
    FC_ASSERT(get_fund_budget_count() < SCORUM_BUDGET_LIMIT_COUNT_FUND_BUDGETS,
              "can't create more then ${1} fund budgets", ("1", SCORUM_BUDGET_LIMIT_COUNT_FUND_BUDGETS));

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();

    time_point_sec start_date = props.time;
    FC_ASSERT(start_date < deadline, "invalid deadline");

    auto head_block_num = db_impl().head_block_num();

    share_type per_block = _calculate_per_block(start_date, deadline, balance_in_scorum.amount);

    const budget_object& new_budget = db_impl().create<budget_object>([&](budget_object& budget) {
        budget.owner = SCORUM_ROOT_POST_PARENT;
        budget.created = start_date;
        budget.deadline = deadline;
        budget.balance = balance_in_scorum;
        budget.per_block = per_block;
        // allocate cash only after next block generation
        budget.last_allocated_block = head_block_num;
    });

    db_impl().modify(props, [&](dynamic_global_property_object& ps) { ps.current_supply -= balance_in_scorum; });

    // there are not schedules for genesis budget
    return new_budget;
}

const budget_object& dbs_budget::create_budget(const account_object& owner,
                                               const asset& balance_in_scorum,
                                               const time_point_sec& deadline,
                                               const optional<string>& content_permlink)
{
    FC_ASSERT(owner.name != SCORUM_ROOT_POST_PARENT, "not allowed for ordinary budget");
    FC_ASSERT(balance_in_scorum.symbol == SCORUM_SYMBOL, "invalid asset type (symbol)");
    FC_ASSERT(balance_in_scorum.amount > 0, "invalid balance_in_scorum");
    FC_ASSERT(owner.balance >= balance_in_scorum, "insufficient funds");
    FC_ASSERT(get_budget_count(owner.name) < SCORUM_BUDGET_LIMIT_COUNT_PER_OWNER,
              "can't create more then ${1} budgets per owner", ("1", SCORUM_BUDGET_LIMIT_COUNT_PER_OWNER));

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();

    time_point_sec start_date = props.time;
    FC_ASSERT(start_date < deadline, "invalid deadline");

    dbs_account& account_service = db().obtain_service<dbs_account>();

    account_service.decrease_balance(owner, balance_in_scorum);

    auto head_block_num = db_impl().head_block_num();

    share_type per_block = _calculate_per_block(start_date, deadline, balance_in_scorum.amount);

    const budget_object& new_budget = db_impl().create<budget_object>([&](budget_object& budget) {
        budget.owner = owner.name;
        if (content_permlink.valid())
        {
            fc::from_string(budget.content_permlink, *content_permlink);
        }
        budget.created = start_date;
        budget.deadline = deadline;
        budget.balance = balance_in_scorum;
        budget.per_block = per_block;
        // allocate cash only after next block generation
        budget.last_allocated_block = head_block_num;
    });

    db_impl().modify(props, [&](dynamic_global_property_object& ps) { ps.current_supply -= balance_in_scorum; });

    return new_budget;
}

void dbs_budget::close_budget(const budget_object& budget)
{
    _close_owned_budget(budget);
}

asset dbs_budget::allocate_cash(const budget_object& budget, const optional<time_point_sec>& now)
{
    asset ret(0, SCORUM_SYMBOL);

    dbs_budget::_time t = _get_now(now);
    auto head_block_num = db_impl().head_block_num();

    if (budget.last_allocated_block >= head_block_num)
    {
        return ret; // empty (allocation waits new block)
    }

    FC_ASSERT(budget.per_block > 0, "invalid per_block");
    ret = _decrease_balance(budget, asset(budget.per_block, SCORUM_SYMBOL));

    if (budget.deadline <= t)
    {
        if (_is_fund_budget(budget))
        {
            // cash back from budget to requesting beneficiary
            // to save from burning (no owner for fund budget)
            ret.amount += budget.balance.amount;
        }
        _close_budget(budget);
    }
    else
    {
        if (!_check_autoclose(budget))
        {
            db_impl().modify(budget, [&](budget_object& b) { b.last_allocated_block = head_block_num; });
        }
    }
    return ret;
}

share_type dbs_budget::_calculate_per_block(const time_point_sec& start_date,
                                            const time_point_sec& end_date,
                                            share_type balance_amount)
{
    FC_ASSERT(start_date.sec_since_epoch() < end_date.sec_since_epoch(), "invalid date interval");

    share_type ret(balance_amount);

    // calculate time interval in seconds.
    // SCORUM_BLOCK_INTERVAL must be in seconds!
    uint32_t delta_in_sec = end_date.sec_since_epoch() - start_date.sec_since_epoch();

    // multiply before division to prevent integer clipping to zero
    ret *= SCORUM_BLOCK_INTERVAL;
    ret /= delta_in_sec;

    // non zero budget must return at least one satoshi
    if (ret < 1)
    {
        ret = 1;
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
        _close_budget(budget);
        return true;
    }
    else
    {
        return false;
    }
}

bool dbs_budget::_is_fund_budget(const budget_object& budget) const
{
    return budget.owner == SCORUM_ROOT_POST_PARENT;
}

void dbs_budget::_close_budget(const budget_object& budget)
{
    if (_is_fund_budget(budget))
    {
        _close_fund_budget(budget);
    }
    else
    {
        _close_owned_budget(budget);
    }
}

void dbs_budget::_close_owned_budget(const budget_object& budget)
{
    FC_ASSERT(!_is_fund_budget(budget), "not allowed for fund budget");

    dbs_account& account_service = db().obtain_service<dbs_account>();

    const auto& owner = account_service.get_account(budget.owner);

    // withdraw all balance rest asset back to owner
    //
    asset repayable = budget.balance;
    if (repayable.amount > 0)
    {
        const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();

        repayable = _decrease_balance(budget, repayable);
        account_service.increase_balance(owner, repayable);

        db_impl().modify(props, [&](dynamic_global_property_object& ps) { ps.current_supply += repayable; });
    }

    // delete budget
    //
    db_impl().remove(budget);
}

void dbs_budget::_close_fund_budget(const budget_object& budget)
{
    FC_ASSERT(_is_fund_budget(budget), "not allowed for ordinary budget");

    // delete budget
    //
    db_impl().remove(budget);
}
}
}
