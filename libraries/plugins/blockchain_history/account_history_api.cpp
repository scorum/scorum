#include <scorum/blockchain_history/account_history_api.hpp>
#include <scorum/blockchain_history/blockchain_history_plugin.hpp>
#include <scorum/blockchain_history/schema/account_history_object.hpp>
#include <scorum/app/api_context.hpp>
#include <scorum/app/application.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>
#include <scorum/common_api/defines.hpp>

#include <map>

namespace scorum {
namespace blockchain_history {

namespace detail {
class account_history_api_impl
{
public:
    scorum::app::application& _app;

public:
    account_history_api_impl(scorum::app::application& app)
        : _app(app)
    {
    }

    template <typename history_object_type>
    std::map<uint32_t, applied_operation> get_history(const std::string& account, uint64_t from, uint32_t limit) const
    {
        const auto db = _app.chain_database();

        FC_ASSERT(limit > 0, "Limit must be greater than zero");
        FC_ASSERT(limit <= MAX_BLOCKCHAIN_HISTORY_DEPTH, "Limit of ${l} is greater than maxmimum allowed ${2}",
                  ("l", limit)("2", MAX_BLOCKCHAIN_HISTORY_DEPTH));
        FC_ASSERT(from >= limit, "From must be greater than limit");

        std::map<uint32_t, applied_operation> result;

        const auto& idx = db->get_index<history_index<history_object_type>>().indices().get<by_account>();
        auto itr = idx.lower_bound(boost::make_tuple(account, from));
        if (itr != idx.end())
        {
            auto end
                = idx.lower_bound(boost::make_tuple(account, std::max(int64_t(0), int64_t(itr->sequence) - limit)));
            while (itr != end)
            {
                result[itr->sequence] = db->get(itr->op);
                ++itr;
            }
        }
        return result;
    }
};
} // namespace detail

account_history_api::account_history_api(const scorum::app::api_context& ctx)
    : _impl(new detail::account_history_api_impl(ctx.app))
{
}

account_history_api::~account_history_api()
{
}

void account_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_scr_to_scr_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    return _impl->_app.chain_database()->with_read_lock(
        [&]() { return _impl->get_history<transfers_to_scr_history_object>(account, from, limit); });
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_scr_to_sp_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    return _impl->_app.chain_database()->with_read_lock(
        [&]() { return _impl->get_history<transfers_to_sp_history_object>(account, from, limit); });
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_history(const std::string& account, uint64_t from, uint32_t limit) const
{
    return _impl->_app.chain_database()->with_read_lock(
        [&]() { return _impl->get_history<account_history_object>(account, from, limit); });
}

} // namespace blockchain_history
} // namespace scorum
