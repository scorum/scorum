#include <scorum/app/advertising_api.hpp>
#include <scorum/app/detail/advertising_api.hpp>

namespace scorum {
namespace app {

advertising_api::advertising_api(const api_context& ctx)
    : _impl(std::make_unique<impl>(*ctx.app.chain_database(),
                                   static_cast<chain::data_service_factory_i&>(*ctx.app.chain_database())))
{
}

advertising_api::~advertising_api() = default;

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
