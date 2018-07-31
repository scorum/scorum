#pragma once

#include <map>
#include <fc/api.hpp>
#include <scorum/blockchain_history/schema/applied_operation.hpp>
#include <scorum/blockchain_history/api_objects.hpp>
#include <scorum/protocol/transaction.hpp>

#ifndef API_BLOCKCHAIN_HISTORY
#define API_BLOCKCHAIN_HISTORY "blockchain_history_api"
#endif

namespace scorum {
namespace app {
struct api_context;
}
} // namespace scorum

namespace scorum {
namespace blockchain_history {

using namespace scorum::protocol;

namespace detail {
class blockchain_history_api_impl;
}

/**
 * @brief Provide set of getters to retrive information about blocks, transactions and operations
 *
 * Require: blockchain_history_plugin
 *
 * @ingroup api
 * @ingroup blockchain_history_plugin
 * @defgroup blockchain_history_api Blockchain history API
 */
class blockchain_history_api
{
public:
    blockchain_history_api(const scorum::app::api_context& ctx);
    ~blockchain_history_api();

    void on_api_startup();

    /// @name Public API
    /// @addtogroup blockchain_history_api
    /// @{

    /**
     * @brief This method returns all operations in ids range [from-limit, from]
     * @param from_op - the operation number, -1 means most recent, limit is the number of operations before from.
     * @param limit - the maximum number of items that can be queried (0 to 100], must be less than from
     * @param type_of_operation Operations type (all = 0, not_virt = 1, virt = 2, market = 3)
     */
    std::map<uint32_t, applied_operation>
    get_ops_history(uint32_t from_op, uint32_t limit, applied_operation_type type_of_operation) const;

    /**
     *  @brief This method returns all operations in timestamp range [from, to]
     *  @param from - the time from start searching operations
     *  @param to - the time until end searching operations
     *  @param from_op - the operation number, -1 means most recent, limit is the number of operations before from.
     *  @param limit - the maximum number of items that can be queried (0 to 100], must be less than from
     */
    std::map<uint32_t, applied_operation> get_ops_history_by_time(const fc::time_point_sec& from,
                                                                  const fc::time_point_sec& to,
                                                                  uint32_t from_op,
                                                                  uint32_t limit) const;

    /**
     * @brief Returns sequence of operations included/generated in a specified block
     * @param block_num Block height of specified block
     * @param type_of_operation Operations type (all = 0, not_virt = 1, virt = 2, market = 3)
     */
    std::map<uint32_t, applied_operation> get_ops_in_block(uint32_t block_num,
                                                           applied_operation_type type_of_operation) const;

    /**
     * @brief This method returns signed transaction by transaction id
     * @param transaction id
     * @return annotated signed transaction
     */
    annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;

    /**
     * @brief Retrieve a block header
     * @param block_num Height of the block whose header should be returned
     * @return header of the referenced block, or null if no matching block was found
     */
    optional<block_header> get_block_header(uint32_t block_num) const;

    /**
     * @brief Retrieve the list of block headers in range [from-limit, from]
     * @param block_num Height of the block to be returned
     * @param limit the maximum number of blocks that can be queried (0 to 100], must be less than from
     * @return the list of block headers
     */
    std::map<uint32_t, block_header> get_block_headers_history(uint32_t block_num, uint32_t limit) const;

    /**
     * @brief Retrieve a full, signed block
     * @param block_num Height of the block to be returned
     * @return the referenced block, or null if no matching block was found
     */
    optional<signed_block_api_obj> get_block(uint32_t block_num) const;

    /**
     * @brief Retrieve the list of signed block from block log (irreversible blocks) in range [from-limit, from]
     * @param block_num Height of the block to be returned
     * @param limit the maximum number of blocks that can be queried (0 to 100], must be less than from
     * @return the list of signed blocks
     */
    std::map<uint32_t, signed_block_api_obj> get_blocks_history(uint32_t block_num, uint32_t limit) const;

    /// @}

private:
    std::unique_ptr<detail::blockchain_history_api_impl> _impl;
};

} // namespace blockchain_history
} // namespace scorum

FC_API(scorum::blockchain_history::blockchain_history_api,
       (get_ops_history)(get_ops_history_by_time)(get_ops_in_block)
       // Blocks and transactions
       (get_transaction)(get_block_header)(get_block_headers_history)(get_block)(get_blocks_history))
