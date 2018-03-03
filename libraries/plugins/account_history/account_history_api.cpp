#include <scorum/account_history/account_history_api.hpp>
#include <scorum/account_history/account_history_plugin.hpp>
#include <scorum/account_history/schema/account_history_object.hpp>
#include <scorum/app/api_context.hpp>
#include <scorum/app/application.hpp>
#include <scorum/chain/schema/operation_object.hpp>
#include <map>

namespace scorum {
namespace account_history {

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

    std::map<uint32_t, applied_operation>
    get_account_history(const std::string& account, uint64_t from, uint32_t limit) const
    {
        const auto db = _app.chain_database();

        FC_ASSERT(limit <= 10000, "Limit of ${l} is greater than maxmimum allowed", ("l", limit));
        FC_ASSERT(from >= limit, "From must be greater than limit");
        //   idump((account)(from)(limit));
        const auto& idx = db->get_index<account_history_index>().indices().get<by_account>();
        auto itr = idx.lower_bound(boost::make_tuple(account, from));
        //   if( itr != idx.end() ) idump((*itr));
        auto end = idx.upper_bound(boost::make_tuple(account, std::max(int64_t(0), int64_t(itr->sequence) - limit)));
        //   if( end != idx.end() ) idump((*end));

        std::map<uint32_t, applied_operation> result;
        while (itr != end)
        {
            result[itr->sequence] = db->get(itr->op);
            ++itr;
        }
        return result;
    }

    std::map<uint32_t, applied_operation>
    get_account_transfers(const std::string& account, uint64_t from, uint32_t limit) const
    {
        // FC_ASSERT(limit <= 10000, "Limit of ${l} is greater than maxmimum allowed", ("l", limit));
        // FC_ASSERT(from >= limit, "From must be greater than limit");
        ////   idump((account)(from)(limit));
        // const auto& idx = my->_db.get_index<account_history_index>().indices().get<by_account>();
        // auto itr = idx.lower_bound(boost::make_tuple(account, from));
        ////   if( itr != idx.end() ) idump((*itr));
        // auto end = idx.upper_bound(boost::make_tuple(account, std::max(int64_t(0), int64_t(itr->sequence) - limit)));
        ////   if( end != idx.end() ) idump((*end));

        std::map<uint32_t, applied_operation> result;
        // while (itr != end)
        //{
        //    result[itr->sequence] = my->_db.get(itr->op);
        //    ++itr;
        //}
        return result;
    }
};
} // namespace detail

account_history_api::account_history_api(const scorum::app::api_context& ctx)
{
    my = std::make_shared<detail::account_history_api_impl>(ctx.app);
}

void account_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    return my->_app.chain_database()->with_read_lock([&]() { return my->get_account_transfers(account, from, limit); });
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_history(const std::string& account, uint64_t from, uint32_t limit) const
{
    return my->_app.chain_database()->with_read_lock([&]() { return my->get_account_history(account, from, limit); });
}

} // namespace account_history
} // namespace scorum
