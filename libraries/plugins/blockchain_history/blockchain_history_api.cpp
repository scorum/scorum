#include <scorum/blockchain_history/blockchain_history_api.hpp>
#include <scorum/app/application.hpp>
#include <scorum/blockchain_history/schema/operation_object.hpp>

namespace scorum {
namespace blockchain_history {

blockchain_history_api::blockchain_history_api(const scorum::app::api_context& ctx)
    : _app(ctx.app)
{
}

void blockchain_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation>
blockchain_history_api::get_history_by_blocks(uint32_t from_block, uint32_t limit, bool only_not_virtual) const
{
    using namespace scorum::chain;

    static const uint32_t max_history_depth = 10000;

    const auto& db = _app.chain_database();

    FC_ASSERT(limit <= max_history_depth, "Limit of ${l} is greater than maxmimum allowed ${2}",
              ("l", limit)("2", max_history_depth));
    FC_ASSERT(from_block >= limit, "From must be greater than limit");

    return db->with_read_lock([&]() {

        std::map<uint32_t, applied_operation> result;

        const auto& idx = db->get_index<operation_index>().indices().get<by_location>();
        auto head_block = db->head_block_num();
        if (from_block > head_block)
            from_block = head_block;
        auto itr = idx.lower_bound(from_block);
        if (itr != idx.end())
        {
            auto start = idx.upper_bound(std::max(int64_t(0), int64_t(itr->block) - limit));
            FC_ASSERT(start != idx.end(), "Invalid range");
            applied_operation temp;
            while (itr != start)
            {
                auto id = itr->id;
                temp = *itr;
                if (!only_not_virtual || !is_virtual_operation(temp.op))
                {
                    FC_ASSERT(id._id >= 0, "Invalid operation_object id");
                    result[(uint32_t)id._id] = temp;
                }
                --itr;
            }
        }
        return result;
    });
}

std::map<uint32_t, applied_operation> blockchain_history_api::get_ops_in_block(uint32_t block_num,
                                                                               bool only_virtual) const
{
    using namespace scorum::chain;

    const auto& db = _app.chain_database();

    return db->with_read_lock([&]() {
        const auto& idx = db->get_index<operation_index>().indices().get<by_location>();
        auto itr = idx.lower_bound(block_num);

        std::map<uint32_t, applied_operation> result;
        applied_operation temp;
        while (itr != idx.end() && itr->block == block_num)
        {
            auto id = itr->id;
            temp = *itr;
            if (!only_virtual || is_virtual_operation(temp.op))
            {
                FC_ASSERT(id._id >= 0, "Invalid operation_object id");
                result[(uint32_t)id._id] = temp;
            }
            ++itr;
        }
        return result;
    });
}

annotated_signed_transaction blockchain_history_api::get_transaction(transaction_id_type id) const
{
    using namespace scorum::chain;

#ifdef SKIP_BY_TX_ID
    FC_ASSERT(false, "This node's operator has disabled operation indexing by transaction_id");
#else

    FC_ASSERT(!_app.is_read_only(), "get_transaction is not available in read-only mode.");

    const auto& db = _app.chain_database();

    return db->with_read_lock([&]() {
        const auto& idx = db->get_index<operation_index>().indices().get<by_transaction_id>();
        auto itr = idx.lower_bound(id);
        if (itr != idx.end() && itr->trx_id == id)
        {
            auto blk = db->fetch_block_by_number(itr->block);
            FC_ASSERT(blk.valid());
            FC_ASSERT(blk->transactions.size() > itr->trx_in_block);
            annotated_signed_transaction result = blk->transactions[itr->trx_in_block];
            result.block_num = itr->block;
            result.transaction_num = itr->trx_in_block;
            return result;
        }
        FC_ASSERT(false, "Unknown Transaction ${t}", ("t", id));
    });
#endif
}
}
}
