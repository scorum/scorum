#include <scorum/blockchain_history/blockchain_history_api.hpp>
#include <scorum/app/application.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>

#include <fc/static_variant.hpp>

namespace scorum {
namespace blockchain_history {

blockchain_history_api::blockchain_history_api(const scorum::app::api_context& ctx)
    : _app(ctx.app)
{
}

void blockchain_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation> blockchain_history_api::get_not_virtual_ops_history(uint32_t from_op,
                                                                                          uint32_t limit) const
{
    using namespace scorum::chain;

    static const uint32_t max_history_depth = 10000;

    const auto& db = _app.chain_database();

    FC_ASSERT(limit > 0, "Limit must be greater than zero");
    FC_ASSERT(limit <= max_history_depth, "Limit of ${l} is greater than maxmimum allowed ${2}",
              ("l", limit)("2", max_history_depth));
    FC_ASSERT(from_op >= limit, "From must be greater than limit");

    return db->with_read_lock([&]() {

        std::map<uint32_t, applied_operation> result;

        const auto& idx = db->get_index<not_virtual_operation_index>().indices().get<by_id>();
        auto itr = idx.end();
        if (!idx.empty())
        {
            // move to last operation object
            --itr;
            if (itr->id._id > from_op)
            {
                itr = idx.lower_bound(from_op);
            }
        }

        if (itr != idx.end())
        {
            auto start = idx.lower_bound(std::max(int64_t(0), int64_t(itr->id._id) - limit));
            FC_ASSERT(start != idx.end(), "Invalid range");
            while (itr != start)
            {
                auto id = itr->id;
                FC_ASSERT(id._id >= 0, "Invalid operation_object id");
                result[(uint32_t)id._id] = db->get(itr->op);
                --itr;
            }
        }
        return result;
    });
}

namespace {

bool operation_type_filter(const operation& op, const applied_operation_type& opt)
{
    switch (opt)
    {
    case applied_operation_type::not_virtual_operation:
        return !is_virtual_operation(op);
    case applied_operation_type::virtual_operation:
        return is_virtual_operation(op);
    case applied_operation_type::market_operation:
        return is_market_operation(op);
    case applied_operation_type::all:
    default:;
    }

    return true;
}
}

std::map<uint32_t, applied_operation> blockchain_history_api::get_ops_in_block(uint32_t block_num,
                                                                               applied_operation_type opt) const
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
            if (operation_type_filter(temp.op, opt))
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
