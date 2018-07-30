#include <scorum/chain/database/database.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/budget_object.hpp>

#include <tuple>

namespace scorum {
namespace chain {

dbs_budget::dbs_budget(database& db)
    : base_service_type(db)
{
}

bool dbs_budget::is_fund_budget_exists() const
{
    return find_by<by_owner_name>(SCORUM_ROOT_POST_PARENT_ACCOUNT) != nullptr;
}

dbs_budget::budget_refs_type dbs_budget::get_budgets() const
{
    budget_refs_type ret;

    const auto& idx = db_impl().get_index<budget_index>().indices();

    for (auto it = idx.cbegin(), it_end = idx.cend(); it != it_end; ++it)
    {
        if (!_is_fund_budget(*it))
            ret.push_back(std::cref(*it));
    }

    return ret;
}

const budget_object& dbs_budget::get_fund_budget() const
{
    auto itr = find_by<by_owner_name>(SCORUM_ROOT_POST_PARENT_ACCOUNT);

    FC_ASSERT(itr != nullptr, "Fund budget does not exist.");

    return *itr;
}

std::set<std::string> dbs_budget::lookup_budget_owners(const std::string& lower_bound_owner_name, uint32_t limit) const
{
    std::set<std::string> result;

    const auto& budgets_by_owner_name = db_impl().get_index<budget_index>().indices().get<by_owner_name>();

    // prepare output if limit > 0
    for (auto itr = budgets_by_owner_name.lower_bound(lower_bound_owner_name);
         limit && itr != budgets_by_owner_name.end(); ++itr)
    {
        if (_is_fund_budget(*itr))
            continue;

        --limit;
        result.insert(itr->owner);
    }
    return result;
}

dbs_budget::budget_refs_type dbs_budget::get_budgets(const account_name_type& owner) const
{
    FC_ASSERT(owner != SCORUM_ROOT_POST_PARENT_ACCOUNT, "Owner name reserved for fund budget.");

    budget_refs_type ret;

    auto it_pair = db_impl().get_index<budget_index>().indices().get<by_owner_name>().equal_range(owner);

    for (auto it = it_pair.first, it_end = it_pair.second; it != it_end; ++it)
    {
        ret.push_back(std::cref(*it));
    }

    return ret;
}

const budget_object& dbs_budget::get_budget(budget_id_type id) const
{
    try
    {
        return get_by<by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const budget_object& dbs_budget::create_fund_budget(const asset& balance, const time_point_sec& deadline)
{
    // clang-format off
    FC_ASSERT(find_by<by_owner_name>(SCORUM_ROOT_POST_PARENT_ACCOUNT) == nullptr, "Recreation of fund budget is not allowed.");
    FC_ASSERT(balance.symbol() == SP_SYMBOL, "Invalid asset type (symbol).");
    FC_ASSERT(balance.amount > 0, "Invalid balance.");
    // clang-format on

    time_point_sec start_date = db_impl().obtain_service<dbs_dynamic_global_property>().get().time;
    FC_ASSERT(start_date < deadline, "Invalid deadline.");

    return _create_budget(SCORUM_ROOT_POST_PARENT_ACCOUNT, balance, start_date, deadline);
}

const budget_object& dbs_budget::create_budget(const account_object& owner,
                                               const asset& balance,
                                               const time_point_sec& deadline,
                                               const optional<std::string>& content_permlink)
{
    dbs_account& account_service = db().obtain_service<dbs_account>();
    dbs_dynamic_global_property& dgp_service = db().obtain_service<dbs_dynamic_global_property>();

    const dynamic_global_property_object& dprops = dgp_service.get();

    // clang-format off
    FC_ASSERT(owner.name != SCORUM_ROOT_POST_PARENT_ACCOUNT, "'${1}' name is not allowed for ordinary budget.",("1", SCORUM_ROOT_POST_PARENT_ACCOUNT));
    FC_ASSERT(balance.symbol() == SCORUM_SYMBOL, "Invalid asset type (symbol).");
    FC_ASSERT(balance.amount > 0, "Invalid balance.");
    FC_ASSERT(owner.balance >= balance, "Insufficient funds.", ("owner.balance", owner.balance)("balance", balance));
    FC_ASSERT(_get_budget_count(owner.name) < (uint32_t)SCORUM_BUDGETS_LIMIT_PER_OWNER, "Can't create more then ${1} budgets per owner.", ("1", SCORUM_BUDGETS_LIMIT_PER_OWNER));

    time_point_sec start_date = dprops.time;
    FC_ASSERT(dprops.circulating_capital > balance, "Invalid balance. Must ${1} > ${2}.", ("1", dprops.circulating_capital)("2", balance));
    // clang-format on

    const budget_object& new_budget = _create_budget(owner.name, balance, start_date, deadline, content_permlink);

    account_service.decrease_balance(owner, balance);

    return new_budget;
}

const budget_object& dbs_budget::_create_budget(const account_name_type& owner,
                                                const asset& balance,
                                                const time_point_sec& start_date,
                                                const time_point_sec& end_date,
                                                const optional<std::string>& content_permlink)
{
    auto per_block = _calculate_per_block(start_date, end_date, balance);

    auto head_block_num = db_impl().head_block_num();
    auto head_block_time = db_impl().head_block_time();

    auto advance = (start_date.sec_since_epoch() - head_block_time.sec_since_epoch()) / SCORUM_BLOCK_INTERVAL;
    auto last_cashout_block = head_block_num + advance;

    return create([&](budget_object& budget) {
        budget.owner = owner;
        if (content_permlink.valid())
        {
            fc::from_string(budget.content_permlink, *content_permlink);
        }
        budget.created = start_date;
        budget.deadline = end_date;
        budget.balance = balance;
        budget.per_block = per_block;
        // allocate cash only after next block generation
        budget.last_cashout_block = last_cashout_block;
    });
}

void dbs_budget::close_budget(const budget_object& budget)
{
    FC_ASSERT(!_is_fund_budget(budget), "Can't close fund budget.");

    _close_owned_budget(budget);
}

asset dbs_budget::allocate_cash(const budget_object& budget)
{
    time_point_sec t = db_impl().head_block_time();
    auto head_block_num = db_impl().head_block_num();

    if (budget.last_cashout_block >= head_block_num)
    {
        return asset(0, budget.balance.symbol()); // empty (allocation waits new block)
    }

    FC_ASSERT(budget.per_block.amount > 0, "Invalid per_block.");
    asset ret = _decrease_balance(budget, budget.per_block);

    if (budget.balance.amount > 0)
    {
        // for fund budget if we have missed blocks we continue payments even after deadline until money is over
        if (t < budget.deadline || _is_fund_budget(budget))
        {
            update(budget, [&](budget_object& b) { b.last_cashout_block = head_block_num; });
            return ret;
        }
    }

    _close_budget(budget);

    return ret;
}

asset dbs_budget::_calculate_per_block(const time_point_sec& start_date,
                                       const time_point_sec& end_date,
                                       const asset& balance)
{
    FC_ASSERT(start_date.sec_since_epoch() < end_date.sec_since_epoch(),
              "Invalid date interval. Start time ${1} must be less end time ${2}", ("1", start_date)("2", end_date));

    auto ret = balance;

    // calculate time interval in seconds.
    // SCORUM_BLOCK_INTERVAL must be in seconds!
    uint32_t delta_in_sec = end_date.sec_since_epoch() - start_date.sec_since_epoch();

    // multiply before division to prevent integer clipping to zero
    ret *= SCORUM_BLOCK_INTERVAL;
    ret /= delta_in_sec;

    // non zero budget must return at least one satoshi
    if (ret.amount < 1)
    {
        ret.amount = 1;
    }

    return ret;
}

asset dbs_budget::_decrease_balance(const budget_object& budget, const asset& balance)
{
    FC_ASSERT(balance.amount > 0, "Invalid balance.");

    asset ret(0, SCORUM_SYMBOL);

    update(budget, [&](budget_object& b) {
        if (b.balance.amount > 0 && balance.amount <= b.balance.amount)
        {
            b.balance -= balance;
            ret = balance;
        }
        else
        {
            ret = b.balance;
            b.balance.amount = 0;
        }
    });

    return ret;
}

bool dbs_budget::_is_fund_budget(const budget_object& budget) const
{
    return budget.owner == SCORUM_ROOT_POST_PARENT_ACCOUNT;
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
    FC_ASSERT(!_is_fund_budget(budget), "Not allowed for fund budget.");

    dbs_account& account_service = db().obtain_service<dbs_account>();

    const auto& owner = account_service.get_account(budget.owner);

    // withdraw all balance rest asset back to owner
    //
    asset repayable = budget.balance;
    if (repayable.amount > 0)
    {
        repayable = _decrease_balance(budget, repayable);
        account_service.increase_balance(owner, repayable);
    }

    // delete budget
    //
    remove(budget);
}

void dbs_budget::_close_fund_budget(const budget_object& budget)
{
    FC_ASSERT(_is_fund_budget(budget), "Not allowed for ordinary budget.");

    // delete budget
    //
    remove(budget);
}

uint64_t dbs_budget::_get_budget_count(const account_name_type& owner) const
{
    return db_impl().get_index<budget_index>().indices().get<by_owner_name>().count(owner);
}

} // namespace chain
} // namespace scorum
