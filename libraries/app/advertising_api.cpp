#include <scorum/app/advertising_api.hpp>
#include <scorum/chain/services/advertising_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>

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
        fc::optional<budget_api_obj> ret;

        switch (type)
        {
        case budget_type::post:
        {
            const auto* budget = _post_budget_service.find(id);
            if (budget)
                ret = budget_api_obj(*budget);
        }
        break;
        case budget_type::banner:
        {
            const auto* budget = _banner_budget_service.find(id);
            if (budget)
                ret = budget_api_obj(*budget);
        }
        break;
        }

        return ret;
    }

    std::vector<int64_t> get_current_winners(budget_type type) const
    {
        return {};
    }

public:
    chain::database& _db;

    chain::advertising_property_service_i& _adv_service;
    chain::account_service_i& _account_service;
    chain::banner_budget_service_i& _banner_budget_service;
    chain::post_budget_service_i& _post_budget_service;
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

std::vector<int64_t> advertising_api::get_current_winners(budget_type type) const
{
    return _impl->_db.with_read_lock([&] { return _impl->get_current_winners(type); });
}

void advertising_api::on_api_startup()
{
    // do nothing
}
}
}