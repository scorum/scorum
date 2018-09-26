#pragma once
#include <scorum/app/scorum_api_objects.hpp>
#include <fc/api.hpp>

#define ADVERTISING_API_NAME "advertising_api"

namespace scorum {
namespace app {

/**
 * @brief Provide api for advertising budgets
 *
 * @ingroup api
 * @addtogroup adv_api Advertising API
 */
class advertising_api
{
public:
    advertising_api(const api_context& ctx);
    ~advertising_api() = default;

    /// @name Public API
    /// @addtogroup adv_api
    /// @{

    /**
     * @brief Get Advertising budgets moderator if exists
     */
    fc::optional<account_api_obj> get_moderator() const;

    /**
     * @brief Get all advertising budgets which belong to provided user. Newer first.
     */
    std::vector<budget_api_obj> get_user_budgets(const std::string& user) const;

    /**
     * @brief Get advertising budget
     */
    fc::optional<budget_api_obj> get_budget(int64_t id, budget_type type) const;

    /**
     * @brief Get winners for particular budget type
     */
    std::vector<budget_api_obj> get_current_winners(budget_type type) const;

    /// @}

    void on_api_startup();

    class impl;

private:
    std::shared_ptr<impl> _impl;
};
}
}

FC_API(scorum::app::advertising_api, (get_moderator)(get_user_budgets)(get_budget)(get_current_winners))
