#pragma once

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>

namespace scorum {
namespace chain {

template <typename PositionWeightList, typename PerBlockByPositionList>
share_type calculate_vcg_cash(const size_t position,
                              const PositionWeightList& coeffs,
                              const PerBlockByPositionList& per_block_list)
{
    FC_ASSERT(coeffs.size() > 0, "Invalid coefficient's list");
    FC_ASSERT(per_block_list.size() == coeffs.size() + 1, "Invalid list of per-block values");
    FC_ASSERT(position < coeffs.size(), "Invalid position");

    const auto top_size = coeffs.size() - 1;
    auto inverted_position = top_size;
    inverted_position -= position;

    uint128_t result;
    share_value_type divider = coeffs.at(position);
    FC_ASSERT(divider > 0, "Invalid coefficient value");
    share_value_type priv_coeff = 0;
    for (size_t ci = 0; ci <= inverted_position; ++ci)
    {
        share_value_type x = coeffs.at(top_size - ci);
        FC_ASSERT(x > 0, "Invalid coefficient value");
        FC_ASSERT(x >= priv_coeff, "Invalid coefficient value");
        share_type per_block = per_block_list.at(top_size - ci + 1);
        FC_ASSERT(per_block.value > 0, "Invalid per-block value");
        uint128_t factor = (x - priv_coeff);
        factor *= per_block.value;
        factor /= 100;
        result += factor;
        priv_coeff = x;
    }
    result *= 100;
    result /= divider;
    return share_type(result.to_uint64());
}

template <typename BudgetService> class base_budget_management_algorithm
{
protected:
    using object_type = typename BudgetService::object_type;

    base_budget_management_algorithm(BudgetService& budget_service, dynamic_global_property_service_i& dgp_service)
        : _budget_service(budget_service)
        , _dgp_service(dgp_service)
    {
    }

public:
    const object_type& create_budget(const account_name_type& owner,
                                     const asset& balance,
                                     const time_point_sec& start_date,
                                     const time_point_sec& end_date,
                                     const std::string& json_metadata)
    {
        FC_ASSERT(balance.amount > 0, "Invalid balance.");
        FC_ASSERT(start_date < end_date, "Invalid dates.");

        auto per_block = calculate_per_block(start_date, end_date, balance);

        auto head_block_time = _dgp_service.head_block_time();

        return _budget_service.create([&](object_type& budget) {
            budget.owner = owner;
#ifndef IS_LOW_MEM
            if (!json_metadata.empty())
            {
                fc::from_string(budget.json_metadata, json_metadata);
            }
#endif
            budget.created = head_block_time;
            budget.start = start_date;
            budget.deadline = end_date;
            budget.balance = balance;
            budget.per_block = per_block;
        });
    }

    // return 'true' if budget has been closed
    bool allocate_cash(const object_type& budget, asset& cash)
    {
        FC_ASSERT(cash.amount > 0, "Invalid cash.");

        cash = decrease_balance(budget, cash);

        if (check_close_conditions(budget))
        {
            close_budget_internal(budget);
            return true;
        }

        return false;
    }

protected:
    asset calculate_per_block(const time_point_sec& start_date, const time_point_sec& end_date, const asset& balance)
    {
        FC_ASSERT(start_date.sec_since_epoch() < end_date.sec_since_epoch(),
                  "Invalid date interval. Start time ${1} must be less end time ${2}",
                  ("1", start_date)("2", end_date));

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

    asset decrease_balance(const object_type& budget, const asset& balance)
    {
        FC_ASSERT(balance.amount > 0, "Invalid balance.");

        asset decreased(0, balance.symbol());

        _budget_service.update(budget, [&](object_type& b) {
            decreased = std::min(b.balance, balance);
            b.balance -= decreased;
        });

        return decreased;
    }

    virtual bool check_close_conditions(const object_type&) = 0;
    virtual void close_budget_internal(const object_type&) = 0;

    BudgetService& _budget_service;
    dynamic_global_property_service_i& _dgp_service;
};

class fund_budget_management_algorithm : public base_budget_management_algorithm<fund_budget_service_i>
{
    using base_class = base_budget_management_algorithm<fund_budget_service_i>;

public:
    fund_budget_management_algorithm() = delete;

    fund_budget_management_algorithm(fund_budget_service_i& budget_service,
                                     dynamic_global_property_service_i& dgp_service)
        : base_budget_management_algorithm<fund_budget_service_i>(budget_service, dgp_service)
    {
    }

    const object_type& create_budget(const account_name_type& owner,
                                     const asset& balance,
                                     const time_point_sec& start_date,
                                     const time_point_sec& end_date,
                                     const std::string& permlink)
    {
        FC_ASSERT(balance.symbol() == SP_SYMBOL, "Invalid asset type (symbol).");

        return base_class::create_budget(owner, balance, start_date, end_date, permlink);
    }

private:
    virtual bool check_close_conditions(const object_type& budget) override
    {
        return budget.balance.amount <= 0;
    }

    void close_budget_internal(const object_type& budget) override
    {
        _budget_service.remove(budget);
    }
};

template <typename BudgetService>
class advertising_budget_management_algorithm : public base_budget_management_algorithm<BudgetService>
{
    using base_class = base_budget_management_algorithm<BudgetService>;

public:
    using object_type = typename BudgetService::object_type;

    advertising_budget_management_algorithm() = delete;

    advertising_budget_management_algorithm(BudgetService& budget_service,
                                            dynamic_global_property_service_i& dgp_service,
                                            account_service_i& account_service)
        : base_budget_management_algorithm<BudgetService>(budget_service, dgp_service)
        , _account_service(account_service)
    {
    }

    const object_type& create_budget(const account_name_type& owner,
                                     const asset& balance,
                                     const time_point_sec& start_date,
                                     const time_point_sec& end_date,
                                     const std::string& permlink)
    {
        FC_ASSERT(balance.symbol() == SCORUM_SYMBOL, "Invalid asset type (symbol).");

        const auto& ret = base_class::create_budget(owner, balance, start_date, end_date, permlink);

        take_cash_from_owner(owner, balance);

        return ret;
    }

    // return 'true' if budget has been closed
    bool cash_back(const object_type& budget, asset& change)
    {
        bool ret = this->allocate_cash(budget, change);
        if (!ret && change.amount > 0)
        {
            give_cash_back_to_owner(budget.owner, change);
        }

        return ret;
    }

    void close_budget(const object_type& budget)
    {
        close_budget_internal(budget);
    }

private:
    virtual bool check_close_conditions(const object_type& budget) override
    {
        if (budget.balance.amount <= 0)
            return true;

        time_point_sec t = this->_dgp_service.head_block_time();

        return t >= budget.deadline;
    }

    void close_budget_internal(const object_type& budget)
    {
        // withdraw all balance rest asset back to owner
        //
        asset repayable = budget.balance;
        if (repayable.amount > 0)
        {
            repayable = this->decrease_balance(budget, repayable);

            give_cash_back_to_owner(budget.owner, repayable);
        }

        this->_budget_service.remove(budget);
    }

    void take_cash_from_owner(const account_name_type& owner, const asset& cash)
    {
        this->_account_service.update(this->_account_service.get_account(owner),
                                      [&](account_object& acnt) { acnt.balance -= cash; });

        this->_dgp_service.update([&](dynamic_global_property_object& p) { p.circulating_capital -= cash; });
    }

    void give_cash_back_to_owner(const account_name_type& owner, const asset& cash)
    {
        this->_account_service.update(this->_account_service.get_account(owner),
                                      [&](account_object& acnt) { acnt.balance += cash; });

        this->_dgp_service.update([&](dynamic_global_property_object& p) { p.circulating_capital += cash; });
    }

    account_service_i& _account_service;
};

using post_budget_management_algorithm = advertising_budget_management_algorithm<post_budget_service_i>;

using banner_budget_management_algorithm = advertising_budget_management_algorithm<banner_budget_service_i>;
}
}
