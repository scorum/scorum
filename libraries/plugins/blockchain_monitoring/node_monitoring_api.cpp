#include <scorum/blockchain_monitoring/node_monitoring_api.hpp>
#include <scorum/blockchain_monitoring/blockchain_monitoring_plugin.hpp>

#include <chrono>

namespace scorum {
namespace blockchain_monitoring {

namespace detail {
class perfomance_timer
{
    typedef std::chrono::high_resolution_clock Clock;
    Clock::time_point _block_start_time = Clock::time_point::min();
    Clock::duration _last_block_processing_duration = Clock::duration::zero();

    void start()
    {
        _block_start_time = Clock::now();
    }

    void stop()
    {
        if (_block_start_time != Clock::time_point::min())
        {
            _last_block_processing_duration = Clock::now() - _block_start_time;
            _block_start_time = Clock::time_point::min();
        }
    }

public:
    perfomance_timer(chain::database& db)
    {
        db.pre_applied_block.connect([&](const signed_block& b) { this->start(); });
        db.applied_block.connect([&](const signed_block& b) { this->stop(); });
    }

    std::chrono::microseconds get_last_block_duration() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(_last_block_processing_duration);
    }
};

//////////////////////////////////////////////////////////////////////////
class node_monitoring_api_impl
{

public:
    scorum::app::application& _app;

    perfomance_timer _timer;

public:
    node_monitoring_api_impl(scorum::app::application& app)
        : _app(app)
        , _timer(*_app.chain_database().get())
    {
    }

    void startup()
    {
    }

    std::shared_ptr<blockchain_monitoring_plugin> get_plugin() const
    {
        auto plugin = _app.get_plugin<blockchain_monitoring_plugin>(blockchain_monitoring_plugin_NAME);

        FC_ASSERT(plugin, "Cann't get " blockchain_monitoring_plugin_NAME " plugin from application.");

        return plugin;
    }
};
} // namespace detail

node_monitoring_api::node_monitoring_api(const scorum::app::api_context& ctx)
{
    _my = std::make_shared<detail::node_monitoring_api_impl>(ctx.app);
}

void node_monitoring_api::on_api_startup()
{
}

uint32_t node_monitoring_api::get_last_block_duration_microseconds() const
{
    return _my->_app.chain_database()->with_read_lock([&]() {
        return std::chrono::duration_cast<std::chrono::microseconds>(_my->_timer.get_last_block_duration()).count();
    });
}

uint32_t node_monitoring_api::get_free_shared_memory_mb() const
{
    return _my->_app.chain_database()->with_read_lock(
        [&]() { return uint32_t(_my->_app.chain_database()->get_free_memory() / (1024 * 1024)); });
}
uint32_t node_monitoring_api::get_total_shared_memory_mb() const
{
    return _my->_app.chain_database()->with_read_lock(
        [&]() { return uint32_t(_my->_app.chain_database()->get_size() / (1024 * 1024)); });
}

} // namespace blockchain_monitoring
} // namespace scorum
