#include <scorum/app/advertising_api.hpp>
#include <scorum/chain/services/advertising_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <boost/range/join.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm/sort.hpp>

namespace scorum {
namespace app {

class advertising_api::impl
{
public:
    impl(chain::database& db)
        : _db(db)
        , _adv_service(_db.advertising_property_service())
        , _account_service(_db.account_service())
        , _banner_budget_service(_db.banner_budget_service())
        , _post_budget_service(_db.post_budget_service())
        , _dyn_props_service(_db.dynamic_global_property_service())
    {
    }

    fc::optional<account_api_obj> get_moderator() const
    {
        auto& adv_property_object = _adv_service.get();

        if (adv_property_object.moderator == SCORUM_MISSING_MODERATOR_ACCOUNT)
            return {};

        auto& moderator_account = _account_service.get_account(adv_property_object.moderator);
        return account_api_obj(moderator_account, _db);
    }

    std::vector<budget_api_obj> get_user_budgets(const std::string& user) const
    {
        auto post_budgets = _post_budget_service.get_budgets(user);
        auto banner_budgets = _banner_budget_service.get_budgets(user);

        std::vector<budget_api_obj> ret;
        ret.reserve(post_budgets.size() + banner_budgets.size());

        boost::transform(post_budgets, std::back_inserter(ret),
                         [](const post_budget_object& o) { return budget_api_obj(o); });
        boost::transform(banner_budgets, std::back_inserter(ret),
                         [](const banner_budget_object& o) { return budget_api_obj(o); });

        boost::sort(ret, [](const budget_api_obj& l, const budget_api_obj& r) { return l.created > r.created; });

        return ret;
    }

    fc::optional<budget_api_obj> get_budget(int64_t id, budget_type type) const
    {
        switch (type)
        {
        case budget_type::post:
            return get_budget(_post_budget_service, id);

        case budget_type::banner:
            return get_budget(_banner_budget_service, id);
        }

        FC_THROW("unreachable");
    }

    std::vector<budget_api_obj> get_current_winners(budget_type type) const
    {
        switch (type)
        {
        case budget_type::post:
            return get_current_winners(_post_budget_service);

        case budget_type::banner:
            return get_current_winners(_banner_budget_service);
        }

        FC_THROW("unreachable");
    }

private:
    template <typename TBudgetService>
    fc::optional<budget_api_obj> get_budget(const TBudgetService& budget_service, int64_t id) const
    {
        using object_type = typename TBudgetService::object_type;

        fc::optional<object_type> ret;

        const auto* budget = budget_service.find(id);
        if (budget)
            ret = object_type(*budget);

        return ret;
    }

    template <typename TBudgetService>
    std::vector<budget_api_obj> get_current_winners(const TBudgetService& budget_service) const
    {
        using object_type = typename TBudgetService::object_type;
        constexpr budget_type budget_type_v = TBudgetService::budget_type_v;

        auto head_block_time = _dyn_props_service.get().time;

        auto budgets = budget_service.get_top_budgets_by_start_time(head_block_time);
        const auto& vcg_coeffs = _adv_service.get().get_vcg_coefficients<budget_type_v>();

        auto winners_count = std::min(vcg_coeffs.size(), budgets.size());

        std::vector<budget_api_obj> ret;
        ret.reserve(winners_count);

        auto cmp = [](const object_type& l, const object_type& r) { return l.per_block.amount > r.per_block.amount; };
        auto min_top_it = budgets.begin() + winners_count;

        std::nth_element(budgets.begin(), min_top_it, budgets.end(), cmp);
        std::sort(budgets.begin(), min_top_it, cmp);
        std::transform(budgets.begin(), min_top_it, std::back_inserter(ret),
                       [](const object_type& o) { return budget_api_obj(o); });

        return ret;
    }

public:
    chain::database& _db;

    chain::advertising_property_service_i& _adv_service;
    chain::account_service_i& _account_service;
    chain::banner_budget_service_i& _banner_budget_service;
    chain::post_budget_service_i& _post_budget_service;
    chain::dynamic_global_property_service_i& _dyn_props_service;
};

advertising_api::advertising_api(const api_context& ctx)
    : _impl(std::make_shared<advertising_api::impl>(*ctx.app.chain_database()))
{
}

fc::optional<account_api_obj> advertising_api::get_moderator() const
{
    return _impl->_db.with_read_lock([&] { return _impl->get_moderator(); });
}

std::vector<budget_api_obj> advertising_api::get_user_budgets(const std::string& user) const
{
    return _impl->_db.with_read_lock([&] { return _impl->get_user_budgets(user); });
}

fc::optional<budget_api_obj> advertising_api::get_budget(int64_t id, budget_type type) const
{
    return _impl->_db.with_read_lock([&] { return _impl->get_budget(id, type); });
}

std::vector<budget_api_obj> advertising_api::get_current_winners(budget_type type) const
{
    return _impl->_db.with_read_lock([&] { return _impl->get_current_winners(type); });
}

void advertising_api::on_api_startup()
{
    // do nothing
}
}
}