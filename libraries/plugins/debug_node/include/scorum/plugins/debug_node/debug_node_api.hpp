
#pragma once

#include <memory>
#include <string>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <scorum/protocol/block.hpp>

#include <scorum/chain/schema/witness_objects.hpp>

namespace scorum {
namespace app {
struct api_context;
}
}

namespace scorum {
namespace plugin {
namespace debug_node {

namespace detail {
class debug_node_api_impl;
}

struct get_dev_key_args
{
    std::string name;
};

struct get_dev_key_result
{
    std::string private_key;
    chain::public_key_type public_key;
};

/**
 * @brief Provide api for direct blocklog access
 *
 * Require: debug_node_plugin
 *
 * @ingroup api
 * @ingroup debug_node_plugin
 * @defgroup debug_node_api Debug node API
 */
class debug_node_api
{
public:
    debug_node_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    /// @name Public API
    /// @addtogroup debug_node_api
    /// @{

    /**
     * @brief Push blocks from existing database.
     */
    uint32_t debug_push_blocks(std::string src_filename, uint32_t count, bool skip_validate_invariants = false);

    /**
     * @brief Generate blocks locally.
     */
    uint32_t debug_generate_blocks(std::string debug_key, uint32_t count);

    /**
     * @brief Generate blocks locally until a specified head block time. Can generate them sparsely.
     */
    uint32_t debug_generate_blocks_until(std::string debug_key,
                                         fc::time_point_sec head_block_time,
                                         bool generate_sparsely = true);

    /**
     * @brief Pop a block from the blockchain, returning it
     */
    fc::optional<scorum::chain::signed_block> debug_pop_block();

    //    /*
    //     * @brief Push an already constructed block onto the blockchain. For use with pop_block to traverse state
    //     block by
    //     * block.
    //     * not implemented
    //     */
    //    void debug_push_block(scorum::chain::signed_block& block);

    scorum::chain::witness_schedule_object debug_get_witness_schedule();

    scorum::chain::hardfork_property_object debug_get_hardfork_property_object();

    //    /**
    //     * @brief Directly manipulate database objects (will undo and re-apply last block with new changes
    //     post-applied).
    //     */
    //    void debug_update_object(fc::variant_object update);

    // fc::variant_object debug_get_edits();

    // void debug_set_edits( fc::variant_object edits );

    /**
     * @brief Set developer key prefix. This prefix only applies to the current API session.
     * (Thus, this method is only useful to websocket-based API clients.)
     * Prefix will be used for debug_get_dev_key() and debug_mine_account().
     */
    void debug_set_dev_key_prefix(std::string prefix);

    /**
     * @brief Get developer key. Use debug_set_key_prefix() to set a prefix if desired.
     */
    get_dev_key_result debug_get_dev_key(get_dev_key_args args);

    //    /**
    //     * @brief Start a node with given initial path.
    //     *
    //     * not implemented
    //     */
    //    void start_node(std::string name, std::string initial_db_path);

    //    /**
    //     * @brief Save the database to disk.
    //     * not implemented
    //     */
    //    void save_db(std::string db_path);

    //    /**
    //     * @brief Stream objects to file.  (Hint:  Create with mkfifo and pipe it to a script)
    //     */
    //    void debug_stream_json_objects(std::string filename);

    //    /**
    //     * @brief Flush streaming file.
    //     */
    //    void debug_stream_json_objects_flush();

    void debug_set_hardfork(uint32_t hardfork_id);

    bool debug_has_hardfork(uint32_t hardfork_id);

    /// @}

private:
    std::shared_ptr<detail::debug_node_api_impl> my;
};
} // namespace debug_node
} // namespace plugin
} // namespace scorum

// clang-format off
FC_REFLECT(scorum::plugin::debug_node::get_dev_key_args, (name))

FC_REFLECT(scorum::plugin::debug_node::get_dev_key_result, (private_key)(public_key))

FC_API(scorum::plugin::debug_node::debug_node_api,
       (debug_push_blocks)
       (debug_generate_blocks)
       (debug_generate_blocks_until)
       (debug_pop_block)
       (debug_set_hardfork)
       (debug_has_hardfork)
       (debug_get_witness_schedule)
       (debug_get_hardfork_property_object)
       (debug_set_dev_key_prefix)
       (debug_get_dev_key))

// clang-format on
