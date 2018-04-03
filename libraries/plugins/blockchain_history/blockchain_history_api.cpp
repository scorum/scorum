#include <scorum/blockchain_history/blockchain_history_api.hpp>
#include <scorum/app/application.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>

#include <fc/static_variant.hpp>

namespace scorum {
namespace blockchain_history {

namespace detail {

class blockchain_history_api_impl
{
public:
    scorum::app::application& _app;

private:
    template <typename ObjectType> applied_operation get_filtered_operation(const ObjectType& obj) const
    {
        const auto& db = _app.chain_database();
        return db->get(obj.op);
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
    {
    }

    using result_type = std::map<uint32_t, applied_operation>;

    template <typename IndexType> result_type get_ops_history(uint32_t from_op, uint32_t limit) const
    {
        using namespace scorum::chain;

        static const uint32_t max_history_depth = 100;

        const auto& db = _app.chain_database();

        FC_ASSERT(limit > 0, "Limit must be greater than zero");
        FC_ASSERT(limit <= max_history_depth, "Limit of ${l} is greater than maxmimum allowed ${2}",
                  ("l", limit)("2", max_history_depth));
        FC_ASSERT(from_op >= limit, "From must be greater than limit");

        return db->with_read_lock([&]() {

            result_type result;

            const auto& idx = db->get_index<IndexType>().indices().get<by_id>();
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
        });
    }
};

bool operation_filter(operation& op, const applied_operation_type& type_of_operation)
{
    switch (type_of_operation)
    {
    case applied_operation_type::market:
        return is_market_operation(op);
    case applied_operation_type::virt:
        return is_virtual_operation(op);
    case applied_operation_type::not_virt:
        return !is_virtual_operation(op);
    default:;
    }

    return true;
}

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
    uint32_t from_op, uint32_t limit, const applied_operation_type& type_of_operation) const
{
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
}

std::map<uint32_t, applied_operation>
blockchain_history_api::get_ops_in_block(uint32_t block_num, applied_operation_type type_of_operation) const
{
    using namespace scorum::chain;

    const auto& db = _impl->_app.chain_database();

    return db->with_read_lock([&]() {
        const auto& idx = db->get_index<operation_index>().indices().get<by_location>();
        auto itr = idx.lower_bound(block_num);

        std::map<uint32_t, applied_operation> result;
        applied_operation temp;
        while (itr != idx.end() && itr->block == block_num)
        {
            auto id = itr->id;
            temp = *itr;
            if (detail::operation_filter(temp.op, type_of_operation))
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

    FC_ASSERT(!_impl->_app.is_read_only(), "get_transaction is not available in read-only mode.");

    const auto& db = _impl->_app.chain_database();

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
