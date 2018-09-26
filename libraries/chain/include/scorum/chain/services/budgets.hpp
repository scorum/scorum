#pragma once
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

class fund_budget_object;
template <budget_type> class adv_budget_object;

namespace detail {
asset calculate_per_block(const time_point_sec& start_date, const time_point_sec& end_date, const asset& balance);
}

struct fund_budget_service_i : public base_service_i<fund_budget_object>
{
    virtual const fund_budget_object&
    create_budget(const asset& balance, fc::time_point_sec start, fc::time_point_sec end)
        = 0;
    virtual asset allocate_cash(const fund_budget_object& budget) = 0;
};

class dbs_fund_budget : public dbs_service_base<fund_budget_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_fund_budget(database& db);

public:
    const fund_budget_object&
    create_budget(const asset& balance, fc::time_point_sec start, fc::time_point_sec end) override;

    asset allocate_cash(const fund_budget_object& budget) override;

private:
    dynamic_global_property_service_i& _dprops_svc;
};

template <budget_type budget_type_v>
struct adv_budget_service_i : public base_service_i<adv_budget_object<budget_type_v>>
{
    using budget_cref_type = typename base_service_i<adv_budget_object<budget_type_v>>::object_cref_type;
    using budgets_type = std::vector<budget_cref_type>;

    virtual const adv_budget_object<budget_type_v>& create_budget(const account_name_type& owner,
                                                                  const asset& balance,
                                                                  fc::time_point_sec start,
                                                                  fc::time_point_sec end,
                                                                  const std::string& json_metadata)
        = 0;

    virtual const adv_budget_object<budget_type_v>& get(const oid<adv_budget_object<budget_type_v>>&) const = 0;
    virtual bool is_exists(const oid<adv_budget_object<budget_type_v>>& id) const = 0;
    virtual const adv_budget_object<budget_type_v>* find(const oid<adv_budget_object<budget_type_v>>&) const = 0;

    virtual budgets_type get_budgets() const = 0;
    virtual budgets_type get_pending_budgets() const = 0;
    virtual budgets_type get_top_budgets(const fc::time_point_sec& until, uint16_t limit) const = 0;
    virtual budgets_type get_top_budgets(const fc::time_point_sec& until) const = 0;
    virtual std::set<std::string> lookup_budget_owners(const std::string& lower_bound_owner_name,
                                                       uint32_t limit) const = 0;
    virtual budgets_type get_budgets(const account_name_type& owner) const = 0;

    virtual asset allocate_cash(const adv_budget_object<budget_type_v>& budget) = 0;
    virtual void update_pending_payouts(const adv_budget_object<budget_type_v>& budget,
                                        const asset& owner_incoming,
                                        const asset& budget_outgoing)
        = 0;

    virtual asset perform_pending_payouts(const budgets_type& budgets) = 0;
    virtual void finish_budget(const oid<adv_budget_object<budget_type_v>>& id) = 0;
    virtual void close_empty_budgets() = 0;
};

struct banner_budget_service_i : public adv_budget_service_i<budget_type::banner>
{
};
struct post_budget_service_i : public adv_budget_service_i<budget_type::post>
{
};

template <budget_type> struct budget_service_traits;
template <> struct budget_service_traits<budget_type::banner>
{
    using service_type = banner_budget_service_i;
};
template <> struct budget_service_traits<budget_type::post>
{
    using service_type = post_budget_service_i;
};

template <budget_type budget_type_v>
class dbs_advertising_budget : public dbs_service_base<typename budget_service_traits<budget_type_v>::service_type>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_advertising_budget(database& db);

public:
    using budgets_type = typename adv_budget_service_i<budget_type_v>::budgets_type;

    const adv_budget_object<budget_type_v>& create_budget(const account_name_type& owner,
                                                          const asset& balance,
                                                          fc::time_point_sec start_date,
                                                          fc::time_point_sec end_date,
                                                          const std::string& json_metadata) override;

    const adv_budget_object<budget_type_v>& get(const oid<adv_budget_object<budget_type_v>>& id) const override;
    bool is_exists(const oid<adv_budget_object<budget_type_v>>& id) const override;
    const adv_budget_object<budget_type_v>* find(const oid<adv_budget_object<budget_type_v>>& id) const override;

    budgets_type get_budgets() const override;
    budgets_type get_budgets(const account_name_type& owner) const override;
    budgets_type get_pending_budgets() const override;

    // TODO: cover these method with unit tests after db_accessors will be introduced
    budgets_type get_top_budgets(const fc::time_point_sec& until, uint16_t limit) const override;
    budgets_type get_top_budgets(const fc::time_point_sec& until) const override;

    std::set<std::string> lookup_budget_owners(const std::string& lower_bound_owner_name,
                                               uint32_t limit) const override;

    asset allocate_cash(const adv_budget_object<budget_type_v>& budget) override;

    void update_pending_payouts(const adv_budget_object<budget_type_v>& budget,
                                const asset& owner_incoming,
                                const asset& budget_outgoing) override;

    asset perform_pending_payouts(const budgets_type& budgets) override;

    void finish_budget(const oid<adv_budget_object<budget_type_v>>& id) override;
    void close_empty_budgets() override;

private:
    dynamic_global_property_service_i& _dprops_svc;
    account_service_i& _account_svc;
};

using dbs_banner_budget = dbs_advertising_budget<budget_type::banner>;
using dbs_post_budget = dbs_advertising_budget<budget_type::post>;

} // namespace chain
} // namespace scorum
