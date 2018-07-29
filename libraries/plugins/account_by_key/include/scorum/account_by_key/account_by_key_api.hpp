#pragma once

#include <scorum/app/application.hpp>

#include <scorum/account_by_key/account_by_key_objects.hpp>

#include <fc/api.hpp>

namespace scorum {
namespace account_by_key {

namespace detail {
class account_by_key_api_impl;
}

/**
 * @brief Allows find account name by its public key.
 *
 * Require: account_by_key_plugin
 *
 * @ingroup api
 * @ingroup account_by_key_plugin
 * @addtogroup account_by_key_api Account by key API
 */
class account_by_key_api
{
public:
    account_by_key_api(const app::api_context& ctx);

    void on_api_startup();

    /// @name Public API
    /// @addtogroup account_by_key_api
    /// @{

    /**
     * @brief get_key_references
     * @param keys
     * @return return array of account names
     */
    std::vector<std::vector<account_name_type>> get_key_references(std::vector<public_key_type> keys) const;

    /// @}

private:
    std::shared_ptr<detail::account_by_key_api_impl> my;
};
}
} // scorum::account_by_key

FC_API(scorum::account_by_key::account_by_key_api, (get_key_references))
