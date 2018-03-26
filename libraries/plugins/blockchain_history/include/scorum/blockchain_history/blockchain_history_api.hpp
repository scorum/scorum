#pragma once

#include <fc/api.hpp>
#include <scorum/app/applied_operation.hpp>
#include <scorum/protocol/transaction.hpp>

namespace scorum {
namespace app {
struct api_context;
class application;
}
} // namespace scorum

namespace scorum {
namespace blockchain_history {

using scorum::app::applied_operation;

class blockchain_history_api
{
public:
    blockchain_history_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    std::map<uint32_t, applied_operation> get_history(uint64_t from, uint32_t limit) const;

    /**
     *  @brief Get sequence of operations included/generated within a particular block
     *  @param block_num Height of the block whose generated virtual operations should be returned
     *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
     *  @return sequence of operations included/generated within the block
     */
    std::vector<applied_operation> get_ops_in_block(uint32_t block_num, bool only_virtual = true) const;

    annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;

private:
    scorum::app::application& _app;
};
} // namespace blockchain_history
} // namespace scorum

FC_API(scorum::account_history::blockchain_history_api, (get_history))
