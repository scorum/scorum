#pragma once

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>

#include <boost/range/algorithm_ext/copy_n.hpp>

namespace scorum {
namespace chain {

template <typename TPerBlockContainer, typename TCoeffsContainer>
std::vector<share_type> calculate_vcg_bets(const TPerBlockContainer& per_block_list,
                                           const TCoeffsContainer& vcg_coefficients)
{
    FC_ASSERT(vcg_coefficients.size() > 0, "invalid coefficient's list");
    FC_ASSERT(per_block_list.size() <= vcg_coefficients.size() + 1, "invalid list of per-block values");
    FC_ASSERT(std::is_sorted(vcg_coefficients.rbegin(), vcg_coefficients.rend()), "per-block list isn't sorted");
    FC_ASSERT(std::is_sorted(per_block_list.rbegin(), per_block_list.rend()), "VCG coefficients aren't sorted");
    FC_ASSERT(*vcg_coefficients.rbegin() > 0, "VCG coefficients should be positive");
    FC_ASSERT(*vcg_coefficients.begin() <= 100, "VCG coefficients should be less than 100");
    FC_ASSERT(per_block_list.empty() || *per_block_list.rbegin() > 0, "per-block amount should be positive");

    int64_t valuable_vcg_coeff_count = std::max((int64_t)per_block_list.size() - 1, 0l);

    std::vector<share_type> vcg_bets;
    vcg_bets.reserve(std::min(per_block_list.size(), vcg_coefficients.size()));

    for (int64_t bet_no = 0l; bet_no < valuable_vcg_coeff_count; ++bet_no)
    {
        uint128_t result = 0;
        percent_type superior_coeff = 0;
        for (int64_t coeff_no = valuable_vcg_coeff_count - 1; coeff_no >= bet_no; --coeff_no)
        {
            auto coeff = vcg_coefficients[coeff_no];
            auto per_block = per_block_list[coeff_no + 1];

            uint128_t factor = coeff - superior_coeff;
            factor *= per_block.value;
            result += factor;
            superior_coeff = coeff;
        }

        result /= vcg_coefficients[bet_no];
        vcg_bets.push_back(result.to_uint64());
    }

    if (valuable_vcg_coeff_count < (int64_t)vcg_coefficients.size() && !per_block_list.empty())
        vcg_bets.push_back(per_block_list[valuable_vcg_coeff_count]);

    return vcg_bets;
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

    asset allocate_cash(const object_type& budget)
    {
        auto result_cash = decrease_balance(budget, budget.per_block);

        if (check_close_conditions(budget))
        {
            close_budget_internal(budget);
        }

        return result_cash;
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

    void cash_back(const account_name_type& owner, const asset& cash)
    {
        FC_ASSERT(cash.amount > 0, "Invalid cash.");

        give_cash_back_to_owner(owner, cash);
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
        this->_account_service.decrease_balance(this->_account_service.get_account(owner), cash);
    }

    void give_cash_back_to_owner(const account_name_type& owner, const asset& cash)
    {
        this->_account_service.increase_balance(this->_account_service.get_account(owner), cash);
    }

    account_service_i& _account_service;
};

using post_budget_management_algorithm = advertising_budget_management_algorithm<post_budget_service_i>;

using banner_budget_management_algorithm = advertising_budget_management_algorithm<banner_budget_service_i>;
}
}
