#pragma once

#include <fc/api.hpp>
#include <scorum/blockchain_history/schema/applied_operation.hpp>
#include <scorum/protocol/transaction.hpp>

#include <map>

namespace scorum {
namespace app {
struct api_context;
class application;
}
} // namespace scorum

namespace scorum {
namespace blockchain_history {

using scorum::protocol::annotated_signed_transaction;
using scorum::protocol::transaction_id_type;

class blockchain_history_api
{
public:
    blockchain_history_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    std::map<uint32_t, applied_operation> get_not_virtual_ops_history(uint32_t from_op, uint32_t limit) const;

    /**
     *  @brief Get sequence of operations included/generated within a particular block
     *  @param block_num Height of the block whose generated virtual operations should be returned
     *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
     *  @return sequence of operations included/generated within the block
     */
    std::map<uint32_t, applied_operation> get_ops_in_block(uint32_t block_num, applied_operation_type opt) const;

    annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;

private:
    scorum::app::application& _app;
};
} // namespace blockchain_history
} // namespace scorum

FC_API(scorum::blockchain_history::blockchain_history_api,
       (get_not_virtual_ops_history)(get_ops_in_block)(get_transaction))
