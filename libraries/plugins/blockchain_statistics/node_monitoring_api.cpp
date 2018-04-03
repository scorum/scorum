#include <scorum/blockchain_statistics/node_monitoring_api.hpp>
#include <scorum/blockchain_statistics/blockchain_statistics_plugin.hpp>

namespace scorum {
namespace blockchain_statistics {

namespace detail {
class node_monitoring_api_impl
{
public:
    scorum::app::application& _app;

public:
    node_monitoring_api_impl(scorum::app::application& app)
        : _app(app)
    {
    }

    void startup()
    {
    }

    std::shared_ptr<blockchain_statistics_plugin> get_plugin() const
    {
        auto plugin = _app.get_plugin<blockchain_statistics_plugin>(BLOCKCHAIN_STATISTICS_PLUGIN_NAME);

        FC_ASSERT(plugin, "Cann't get " BLOCKCHAIN_STATISTICS_PLUGIN_NAME " plugin from application.");

        return plugin;
    }
};
} // namespace detail

node_monitoring_api::node_monitoring_api(const scorum::app::api_context& ctx)
{
    my = std::make_shared<detail::node_monitoring_api_impl>(ctx.app);
}

void node_monitoring_api::on_api_startup()
{
}

uint32_t node_monitoring_api::get_last_block_duration_microseconds() const
{
    return my->_app.chain_database()->with_read_lock(
        [&]() { return my->get_plugin()->get_last_block_duration_microseconds(); });
}

} // namespace blockchain_statistics
} // namespace scorum
