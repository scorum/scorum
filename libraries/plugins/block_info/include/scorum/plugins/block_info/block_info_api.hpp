
#pragma once

#include <fc/api.hpp>

#include <scorum/plugins/block_info/block_info.hpp>

namespace scorum {
namespace app {
struct api_context;
}
}

namespace scorum {
namespace plugin {
namespace block_info {

namespace detail {
class block_info_api_impl;
}

struct get_block_info_args
{
    uint32_t start_block_num = 0;
    uint32_t count = 1000;
};

/**
 * @brief Provide api to get information about blocks
 *
 * Require: block_info_plugin
 *
 * @ingroup api
 * @ingroup block_info_plugin
 * @defgroup block_info_api Block info API
 */
class block_info_api
{
public:
    block_info_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    /// @name Public API
    /// @addtogroup block_info_api
    /// @{

    std::vector<block_info> get_block_info(get_block_info_args args);
    std::vector<block_with_info> get_blocks_with_info(get_block_info_args args);

    /// @}

private:
    std::shared_ptr<detail::block_info_api_impl> my;
};
}
}
}

FC_REFLECT(scorum::plugin::block_info::get_block_info_args, (start_block_num)(count))

FC_API(scorum::plugin::block_info::block_info_api, (get_block_info)(get_blocks_with_info))
