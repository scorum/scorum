#include <scorum/blockchain_statistics/blockchain_statistics_api.hpp>
#include <scorum/blockchain_statistics/blockchain_statistics_plugin.hpp>
#include <scorum/common_statistic/base_api_impl.hpp>

namespace scorum {
namespace blockchain_statistics {

namespace detail {
class blockchain_statistics_api_impl : public common_statistics::common_statistics_api_impl<bucket_object, statistics>
{
public:
    blockchain_statistics_api_impl(scorum::app::application& app)
        : base_api_impl(app, BLOCKCHAIN_STATISTICS_PLUGIN_NAME)
    {
    }
};
}

blockchain_statistics_api::blockchain_statistics_api(const scorum::app::api_context& ctx)
{
    my = std::make_shared<detail::blockchain_statistics_api_impl>(ctx.app);
}

void blockchain_statistics_api::on_api_startup()
{
}

statistics blockchain_statistics_api::get_stats_for_time(fc::time_point_sec open, uint32_t interval) const
{
    return my->_app.chain_database()->with_read_lock([&]() { return my->get_stats_for_time(open, interval); });
}

statistics blockchain_statistics_api::get_stats_for_interval(fc::time_point_sec start, fc::time_point_sec end) const
{
    return my->_app.chain_database()->with_read_lock(
        [&]() { return my->get_stats_for_interval<blockchain_statistics_plugin>(start, end); });
}

statistics blockchain_statistics_api::get_lifetime_stats() const
{
    return my->_app.chain_database()->with_read_lock([&]() { return my->get_lifetime_stats(); });
}
}
} // scorum::blockchain_statistics
