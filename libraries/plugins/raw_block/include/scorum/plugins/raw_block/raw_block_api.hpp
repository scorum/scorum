
#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <fc/api.hpp>

namespace scorum {
namespace app {
struct api_context;
}
}

namespace scorum {
namespace plugin {
namespace raw_block {

namespace detail {
class raw_block_api_impl;
}

struct get_raw_block_args
{
    uint32_t block_num = 0;
};

struct get_raw_block_result
{
    chain::block_id_type block_id;
    chain::block_id_type previous;
    fc::time_point_sec timestamp;
    std::string raw_block;
};

/**
 * @brief The raw_block_api class
 *
 * Require: raw_block_plugin
 *
 * @ingroup api
 * @ingroup raw_block_plugin
 * @defgroup raw_block_api Raw block API
 */
class raw_block_api
{
public:
    raw_block_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    /// @name Public API
    /// @addtogroup raw_block_api
    /// @{

    get_raw_block_result get_raw_block(get_raw_block_args args);
    void push_raw_block(std::string block_b64);

    /// @}

private:
    std::shared_ptr<detail::raw_block_api_impl> my;
};
}
}
}

FC_REFLECT(scorum::plugin::raw_block::get_raw_block_args, (block_num))

FC_REFLECT(scorum::plugin::raw_block::get_raw_block_result, (block_id)(previous)(timestamp)(raw_block))

FC_API(scorum::plugin::raw_block::raw_block_api, (get_raw_block)(push_raw_block))
