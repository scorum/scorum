#include <scorum/blockchain_history/blockchain_history_api.hpp>
#include <scorum/app/application.hpp>

#define MAX_HISTORY_DEPTH 10000

namespace scorum {
namespace blockchain_history {

blockchain_history_api::blockchain_history_api(const scorum::app::api_context& ctx)
    : _app(ctx.app)
{
}

void blockchain_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation> blockchain_history_api::get_history(uint64_t from, uint32_t limit) const
{
    return _app.chain_database()->with_read_lock([&]() {

        // TODO
        return {};
    });
}

std::vector<applied_operation> blockchain_history_api::get_ops_in_block(uint32_t block_num, bool only_virtual) const
{
    const auto& db = _app.chain_database();

    const auto& idx = db.get_index<operation_index>().indices().get<by_location>();
    auto itr = idx.lower_bound(block_num);
    std::vector<applied_operation> result;
    applied_operation temp;
    while (itr != idx.end() && itr->block == block_num)
    {
        temp = *itr;
        if (!only_virtual || is_virtual_operation(temp.op))
            result.push_back(temp);
        ++itr;
    }
    return result;
}

annotated_signed_transaction blockchain_history_api::get_transaction(transaction_id_type id) const
{
#ifdef SKIP_BY_TX_ID
    FC_ASSERT(false, "This node's operator has disabled operation indexing by transaction_id");
#else
    if (_app.is_read_only())
    {
        return _app.get_write_node_database_api()->get_transaction(id);
    }
    else
    {
        return my->_db.with_read_lock([&]() {
            const auto& idx = my->_db.get_index<operation_index>().indices().get<by_transaction_id>();
            auto itr = idx.lower_bound(id);
            if (itr != idx.end() && itr->trx_id == id)
            {
                auto blk = my->_db.fetch_block_by_number(itr->block);
                FC_ASSERT(blk.valid());
                FC_ASSERT(blk->transactions.size() > itr->trx_in_block);
                annotated_signed_transaction result = blk->transactions[itr->trx_in_block];
                result.block_num = itr->block;
                result.transaction_num = itr->trx_in_block;
                return result;
            }
            FC_ASSERT(false, "Unknown Transaction ${t}", ("t", id));
        });
    }
#endif
}
}
