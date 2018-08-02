
#pragma once

#include <scorum/protocol/types.hpp>

#include <fc/api.hpp>
#include <fc/crypto/sha256.hpp>

#include <string>

namespace scorum {
namespace app {
struct api_context;
}
}

namespace scorum {
namespace plugin {
namespace auth_util {

namespace detail {
class auth_util_api_impl;
}

struct check_authority_signature_params
{
    std::string account_name;
    std::string level;
    fc::sha256 dig;
    std::vector<protocol::signature_type> sigs;
};

struct check_authority_signature_result
{
    std::vector<protocol::public_key_type> keys;
};

/**
 * @brief The auth_util_api class
 *
 * Require: auth_util_plugin
 *
 * @ingroup api
 * @ingroup auth_util_plugin
 * @defgroup auth_util_api Auth util API
 */
class auth_util_api
{
public:
    auth_util_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    /// @name Public API
    /// @addtogroup auth_util_api
    /// @{

    /**
     * @brief check_authority_signature
     * @param args
     * @return return array of public keys
     */
    check_authority_signature_result check_authority_signature(check_authority_signature_params args);

    /// @}

private:
    std::shared_ptr<detail::auth_util_api_impl> my;
};
}
}
}

FC_REFLECT(scorum::plugin::auth_util::check_authority_signature_params, (account_name)(level)(dig)(sigs))
FC_REFLECT(scorum::plugin::auth_util::check_authority_signature_result, (keys))

FC_API(scorum::plugin::auth_util::auth_util_api, (check_authority_signature))
