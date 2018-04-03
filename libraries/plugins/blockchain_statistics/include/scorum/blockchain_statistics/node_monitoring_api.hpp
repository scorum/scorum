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
namespace blockchain_statistics {

namespace detail {
class node_monitoring_api_impl;
}

class node_monitoring_api
{
public:
    node_monitoring_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    /**
    * @brief Returns last block processing duration in microseconds.
    */
    uint32_t get_last_block_duration_microseconds() const;

private:
    std::shared_ptr<detail::node_monitoring_api_impl> my;
};
} // namespace blockchain_statistics
} // namespace scorum

FC_API(scorum::blockchain_statistics::node_monitoring_api, (get_last_block_duration_microseconds))
