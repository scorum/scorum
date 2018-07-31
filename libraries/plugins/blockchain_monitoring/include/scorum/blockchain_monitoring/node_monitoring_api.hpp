#pragma once

#include <fc/api.hpp>

#ifndef API_NODE_MONITORING
#define API_NODE_MONITORING "node_monitoring_api"
#endif

namespace scorum {
namespace app {
struct api_context;
}
} // namespace scorum

namespace scorum {
namespace blockchain_monitoring {

namespace detail {
class node_monitoring_api_impl;
}

/**
 * @brief Node monitoring API
 *
 * Require: blockchain_monitoring_plugin
 *
 * @ingroup api
 * @ingroup blockchain_monitoring_plugin
 * @defgroup node_monitoring_api Node monitoring API
 */
class node_monitoring_api
{
public:
    node_monitoring_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    /// @name Public API
    /// @addtogroup node_monitoring_api
    /// @{

    /**
    * @brief Returns last block processing duration in microseconds.
    */
    uint32_t get_last_block_duration_microseconds() const;

    uint32_t get_free_shared_memory_mb() const;
    uint32_t get_total_shared_memory_mb() const;

    /// @}

private:
    std::shared_ptr<detail::node_monitoring_api_impl> _my;
};
} // namespace blockchain_monitoring
} // namespace scorum

FC_API(scorum::blockchain_monitoring::node_monitoring_api,
       (get_last_block_duration_microseconds)(get_free_shared_memory_mb)(get_total_shared_memory_mb))
