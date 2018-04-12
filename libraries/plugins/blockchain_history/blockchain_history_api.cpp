#include <scorum/blockchain_history/blockchain_history_api.hpp>
#include <scorum/blockchain_history/blockchain_history_plugin.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>
#include <scorum/app/application.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/common_api/config.hpp>

#include <fc/static_variant.hpp>

namespace scorum {
namespace blockchain_history {

namespace detail {

class blockchain_history_api_impl
{
public:
    scorum::app::application& _app;
    std::shared_ptr<chain::database> _db;

private:
    template <typename ObjectType> applied_operation get_filtered_operation(const ObjectType& obj) const
    {
        const auto& _db = _app.chain_database();
        return _db->get(obj.op);
    }

    applied_operation get_operation(const filtered_not_virt_operations_history_object& obj) const
    {
        return get_filtered_operation(obj);
    }

    applied_operation get_operation(const filtered_virt_operations_history_object& obj) const
    {
        return get_filtered_operation(obj);
    }

    applied_operation get_operation(const filtered_market_operations_history_object& obj) const
    {
        return get_filtered_operation(obj);
    }

    applied_operation get_operation(const operation_object& obj) const
    {
        applied_operation temp;
        temp = obj;
        return temp;
    }

public:
    blockchain_history_api_impl(scorum::app::application& app)
        : _app(app)
        , _db(_app.chain_database())
    {
    }

    using result_type = std::map<uint32_t, applied_operation>;

    template <typename IndexType> result_type get_ops_history(uint32_t from_op, uint32_t limit) const
    {
        FC_ASSERT(limit > 0, "Limit must be greater than zero");
        FC_ASSERT(limit <= MAX_BLOCKCHAIN_HISTORY_DEPTH, "Limit of ${l} is greater than maxmimum allowed ${2}",
                  ("l", limit)("2", MAX_BLOCKCHAIN_HISTORY_DEPTH));
        FC_ASSERT(from_op >= limit, "From must be greater than limit");

        result_type result;

        const auto& idx = _db->get_index<IndexType>().indices().get<by_id>();
        if (idx.empty())
            return result;

        // move to last operation object
        auto itr = idx.end();
        --itr;
        if (itr->id._id > from_op)
        {
            itr = idx.lower_bound(from_op);
        }

        auto start = idx.lower_bound(std::max(int64_t(0), int64_t(itr->id._id) - limit));
        FC_ASSERT(start != idx.end(), "Invalid range");
        while (itr != start)
        {
            auto id = itr->id;
            FC_ASSERT(id._id >= 0, "Invalid operation_object id");
            result[(uint32_t)id._id] = get_operation(*itr);
            --itr;
        }
        return result;
    }

    template <typename Filter> result_type get_ops_in_block(uint32_t block_num, Filter operation_filter) const
    {
        const auto& idx = _db->get_index<operation_index>().indices().get<by_location>();
        auto itr = idx.lower_bound(block_num);

        result_type result;

        applied_operation temp;
        while (itr != idx.end() && itr->block == block_num)
        {
            auto id = itr->id;
            temp = *itr;
            if (operation_filter(temp.op))
            {
                FC_ASSERT(id._id >= 0, "Invalid operation_object id");
                result[(uint32_t)id._id] = temp;
            }
            ++itr;
        }
        return result;
    }

    // Blocks and transactions
    annotated_signed_transaction get_transaction(transaction_id_type id) const
    {

#ifdef SKIP_BY_TX_ID
        FC_ASSERT(false, "This node's operator has disabled operation indexing by transaction_id");
#else
        FC_ASSERT(!_app.is_read_only(), "get_transaction is not available in read-only mode.");

        const auto& idx = _db->get_index<operation_index>().indices().get<by_transaction_id>();
        auto itr = idx.lower_bound(id);
        if (itr != idx.end() && itr->trx_id == id)
        {
            auto blk = _db->fetch_block_by_number(itr->block);
            FC_ASSERT(blk.valid());
            FC_ASSERT(blk->transactions.size() > itr->trx_in_block);
            annotated_signed_transaction result = blk->transactions[itr->trx_in_block];
            result.block_num = itr->block;
            result.transaction_num = itr->trx_in_block;
            return result;
        }
        FC_ASSERT(false, "Unknown Transaction ${t}", ("t", id));
#endif
    }

    optional<signed_block> get_block(uint32_t block_num) const
    {
        return _db->fetch_block_by_number(block_num);
    }

    template <typename T> std::map<uint32_t, T> get_blocks_history_by_number(uint32_t block_num, uint32_t limit) const
    {
        FC_ASSERT(limit <= MAX_BLOCKS_HISTORY_DEPTH, "Limit of ${l} is greater than maxmimum allowed ${2}",
                  ("l", limit)("2", MAX_BLOCKS_HISTORY_DEPTH));
        FC_ASSERT(limit > 0, "Limit must be greater than zero");
        FC_ASSERT(block_num >= limit, "From must be greater than limit");

        try
        {
            const dynamic_global_property_object& dgpo = _db->obtain_service<dbs_dynamic_global_property>().get();

            uint32_t last_irreversible_block_num = dgpo.last_irreversible_block_num;
            if (block_num > last_irreversible_block_num)
            {
                block_num = last_irreversible_block_num;
            }

            uint32_t from_block_num = block_num - limit;

            std::map<uint32_t, T> result;
            optional<signed_block> b;
            while (from_block_num != block_num)
            {
                b = _db->read_block_by_number(block_num);
                if (b.valid())
                    result[block_num] = *b; // convert from signed_block to type T
                --block_num;
            }

            return result;
        }
        FC_LOG_AND_RETHROW()
    }
};

} // namespace detail

blockchain_history_api::blockchain_history_api(const scorum::app::api_context& ctx)
    : _impl(new detail::blockchain_history_api_impl(ctx.app))
{
}

blockchain_history_api::~blockchain_history_api()
{
}

void blockchain_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation> blockchain_history_api::get_ops_history(
    uint32_t from_op, uint32_t limit, applied_operation_type type_of_operation) const
{
    return _impl->_app.chain_database()->with_read_lock([&]() {
        switch (type_of_operation)
        {
        case applied_operation_type::not_virt:
            return _impl->get_ops_history<filtered_not_virt_operations_history_index>(from_op, limit);
        case applied_operation_type::virt:
            return _impl->get_ops_history<filtered_virt_operations_history_index>(from_op, limit);
        case applied_operation_type::market:
            return _impl->get_ops_history<filtered_market_operations_history_index>(from_op, limit);
        default:;
        }

        return _impl->get_ops_history<operation_index>(from_op, limit);
    });
}

std::map<uint32_t, applied_operation>
blockchain_history_api::get_ops_in_block(uint32_t block_num, applied_operation_type type_of_operation) const
{
    return _impl->_app.chain_database()->with_read_lock([&]() {
        switch (type_of_operation)
        {
        case applied_operation_type::market:
            return _impl->get_ops_in_block(block_num, is_market_operation);
        case applied_operation_type::virt:
            return _impl->get_ops_in_block(block_num, is_virtual_operation);
        case applied_operation_type::not_virt:
            return _impl->get_ops_in_block(block_num, [&](const operation& op) { return !is_virtual_operation(op); });
        default:;
        }

        return _impl->get_ops_in_block(block_num, [&](const operation& op) { return true; });
    });
}

annotated_signed_transaction blockchain_history_api::get_transaction(transaction_id_type id) const
{
    return _impl->_app.chain_database()->with_read_lock([&]() { return _impl->get_transaction(id); });
}

//////////////////////////////////////////////////////////////////////
// Blocks and transactions                                          //
//////////////////////////////////////////////////////////////////////

optional<block_header> blockchain_history_api::get_block_header(uint32_t block_num) const
{
    FC_ASSERT(!_impl->_app.get_plugin<blockchain_history_plugin>(BLOCKCHAIN_HISTORY_PLUGIN_NAME)->get_block_disabled(),
              "get_block_header is disabled on this node.");

    return _impl->_db->with_read_lock([&]() { return _impl->get_block(block_num); });
}

optional<signed_block_api_obj> blockchain_history_api::get_block(uint32_t block_num) const
{
    FC_ASSERT(!_impl->_app.get_plugin<blockchain_history_plugin>(BLOCKCHAIN_HISTORY_PLUGIN_NAME)->get_block_disabled(),
              "get_block is disabled on this node.");

    return _impl->_db->with_read_lock([&]() { return _impl->get_block(block_num); });
}

std::map<uint32_t, block_header> blockchain_history_api::get_block_headers_history(uint32_t block_num,
                                                                                   uint32_t limit) const
{
    FC_ASSERT(!_impl->_app.is_read_only(), "Disabled for read only mode");
    return _impl->_db->with_read_lock(
        [&]() { return _impl->get_blocks_history_by_number<block_header>(block_num, limit); });
}

std::map<uint32_t, signed_block_api_obj> blockchain_history_api::get_blocks_history(uint32_t block_num,
                                                                                    uint32_t limit) const
{
    FC_ASSERT(!_impl->_app.is_read_only(), "Disabled for read only mode");
    return _impl->_db->with_read_lock(
        [&]() { return _impl->get_blocks_history_by_number<signed_block_api_obj>(block_num, limit); });
}
}
}
