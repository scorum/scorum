#include <scorum/blockchain_history/blockchain_history_api.hpp>
#include <scorum/app/application.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>

#include <fc/static_variant.hpp>

namespace scorum {
namespace blockchain_history {

namespace detail {

using result_type = std::map<uint32_t, applied_operation>;

class blockchain_history_api_impl
{
public:
    scorum::app::application& _app;

public:
    blockchain_history_api_impl(scorum::app::application& app)
        : _app(app)
    {
    }

    template <applied_operation_type T> result_type get_ops_history(uint32_t from_op, uint32_t limit) const
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

            const auto& idx = db->get_index<filtered_operation_index<T>>().indices().get<by_id>();
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
                result[(uint32_t)id._id] = db->get(itr->op);
                --itr;
            }
            return result;
        });
    }
};

struct get_ops_history_visitor
{
    get_ops_history_visitor(const detail::blockchain_history_api_impl& impl, uint32_t from_op, uint32_t limit)
        : _impl(impl)
        , _from_op(from_op)
        , _limit(limit)
    {
    }

    using result_type = detail::result_type;

    result_type operator()(const applied_operation_all&) const
    {
        return _impl.get_ops_history<applied_operation_type::all>(_from_op, _limit);
    }

    result_type operator()(const applied_operation_not_virt&) const
    {
        return _impl.get_ops_history<applied_operation_type::not_virt>(_from_op, _limit);
    }

    result_type operator()(const applied_operation_virt&) const
    {
        return _impl.get_ops_history<applied_operation_type::virt>(_from_op, _limit);
    }

    result_type operator()(const applied_operation_market&) const
    {
        return _impl.get_ops_history<applied_operation_type::market>(_from_op, _limit);
    }

private:
    const detail::blockchain_history_api_impl& _impl;
    uint32_t _from_op;
    uint32_t _limit;
};

struct operation_filter_visitor
{
    operation_filter_visitor(const operation& op)
        : _op(op)
    {
    }

    using result_type = bool;

    result_type operator()(const applied_operation_all&) const
    {
        return true;
    }

    result_type operator()(const applied_operation_not_virt&) const
    {
        return !is_virtual_operation(_op);
    }

    result_type operator()(const applied_operation_virt&) const
    {
        return is_virtual_operation(_op);
    }

    result_type operator()(const applied_operation_market&) const
    {
        return is_market_operation(_op);
    }

private:
    const operation& _op;
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

std::map<uint32_t, applied_operation>
blockchain_history_api::get_ops_history(uint32_t from_op, uint32_t limit, const applied_operation_type& opt) const
{
    return get_applied_operation_variant(opt).visit(detail::get_ops_history_visitor(*_impl, from_op, limit));
}

std::map<uint32_t, applied_operation> blockchain_history_api::get_ops_in_block(uint32_t block_num,
                                                                               applied_operation_type opt) const
{
    using namespace scorum::chain;

    applied_operation_variant_type opt_v = get_applied_operation_variant(opt);

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
            if (opt_v.visit(detail::operation_filter_visitor(temp.op)))
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
