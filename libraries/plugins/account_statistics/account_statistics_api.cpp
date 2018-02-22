#include <scorum/account_statistics/account_statistics_api.hpp>
#include <scorum/account_statistics/account_statistics_plugin.hpp>
#include <scorum/common_statistic/base_api_impl.hpp>

namespace scorum {
namespace account_statistics {

namespace detail {
class account_statistics_api_impl : public common_statistics::common_statistics_api_impl<bucket_object, statistics>
{
public:
    account_statistics_api_impl(scorum::app::application& app)
        : base_api_impl(app, ACCOUNT_STATISTICS_PLUGIN_NAME)
    {
    }
};
} // detail

account_statistics_api::account_statistics_api(const scorum::app::api_context& ctx)
{
    my = std::make_shared<detail::account_statistics_api_impl>(ctx.app);
}

void account_statistics_api::on_api_startup()
{
}

statistics account_statistics_api::get_stats_for_time(fc::time_point_sec open, uint32_t interval) const
{
    return my->_app.chain_database()->with_read_lock([&]() { return my->get_stats_for_time(open, interval); });
}

statistics account_statistics_api::get_stats_for_interval(fc::time_point_sec start, fc::time_point_sec end) const
{
    return my->_app.chain_database()->with_read_lock(
        [&]() { return my->get_stats_for_interval<account_statistics_plugin>(start, end); });
}

statistics account_statistics_api::get_lifetime_stats() const
{
    return my->_app.chain_database()->with_read_lock([&]() { return my->get_lifetime_stats(); });
}
}
} // scorum::account_statistics
