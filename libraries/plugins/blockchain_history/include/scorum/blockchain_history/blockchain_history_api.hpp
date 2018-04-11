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

namespace detail {
class blockchain_history_api_impl;
}

class blockchain_history_api
{
public:
    blockchain_history_api(const scorum::app::api_context& ctx);
    ~blockchain_history_api();

    void on_api_startup();

    std::map<uint32_t, applied_operation>
    get_ops_history(uint32_t from_op, uint32_t limit, const applied_operation_type& type_of_operation) const;

    /** Returns sequence of operations included/generated in a specified block
    *
    * @param block_num Block height of specified block
    * @param type_of_operation Operations type (all = 0, not_virt = 1, virt = 2, market = 3)
    */
    std::map<uint32_t, applied_operation> get_ops_in_block(uint32_t block_num,
                                                           applied_operation_type type_of_operation) const;

    annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;

private:
    std::unique_ptr<detail::blockchain_history_api_impl> _impl;
};
} // namespace blockchain_history
} // namespace scorum

FC_API(scorum::blockchain_history::blockchain_history_api, (get_ops_history)(get_ops_in_block)(get_transaction))
