#pragma once
#include <scorum/chain/services/advertising_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/app/scorum_api_objects.hpp>
#include <scorum/app/advertising_api.hpp>

#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm/sort.hpp>

namespace scorum {
namespace app {
class advertising_api::impl
{
public:
    impl(chain::database& db, chain::data_service_factory_i& services)
        : _db(db)
        , _services(services)
        , _adv_service(_services.advertising_property_service())
        , _account_service(_services.account_service())
        , _banner_budget_service(_services.banner_budget_service())
        , _post_budget_service(_services.post_budget_service())
        , _dyn_props_service(_services.dynamic_global_property_service())
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

    fc::optional<budget_api_obj> get_budget(const uuid_type& uuid, budget_type type) const
    {
        switch (type)
        {
        case budget_type::post:
            return get_budget(_post_budget_service, uuid);

        case budget_type::banner:
            return get_budget(_banner_budget_service, uuid);
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

    std::vector<percent_type> get_auction_coefficients(budget_type type) const
    {
        switch (type)
        {
        case budget_type::post:
            return std::vector<percent_type>{ _adv_service.get().auction_post_coefficients.begin(),
                                              _adv_service.get().auction_post_coefficients.end() };

        case budget_type::banner:
            return std::vector<percent_type>{ _adv_service.get().auction_banner_coefficients.begin(),
                                              _adv_service.get().auction_banner_coefficients.end() };
        }

        FC_THROW("unreachable");
    }

private:
    template <budget_type budget_type_v>
    fc::optional<budget_api_obj> get_budget(const chain::adv_budget_service_i<budget_type_v>& budget_service,
                                            const uuid_type& uuid) const
    {
        fc::optional<budget_api_obj> ret;

        if (budget_service.is_exists(uuid))
            ret = budget_service.get(uuid);

        return ret;
    }

    template <budget_type budget_type_v>
    std::vector<budget_api_obj>
    get_current_winners(const chain::adv_budget_service_i<budget_type_v>& budget_service) const
    {
        using object_type = chain::adv_budget_object<budget_type_v>;

        auto head_block_time = _dyn_props_service.get().time;
        const auto& auction_coeffs = _adv_service.get().get_auction_coefficients<budget_type_v>();

        auto budgets = budget_service.get_top_budgets(head_block_time, auction_coeffs.size());

        std::vector<budget_api_obj> ret;
        ret.reserve(auction_coeffs.size());

        boost::transform(budgets, std::back_inserter(ret), [](const object_type& o) { return budget_api_obj(o); });

        return ret;
    }

public:
    chain::database& _db;

    chain::data_service_factory_i& _services;
    chain::advertising_property_service_i& _adv_service;
    chain::account_service_i& _account_service;
    chain::banner_budget_service_i& _banner_budget_service;
    chain::post_budget_service_i& _post_budget_service;
    chain::dynamic_global_property_service_i& _dyn_props_service;
};
}
}
