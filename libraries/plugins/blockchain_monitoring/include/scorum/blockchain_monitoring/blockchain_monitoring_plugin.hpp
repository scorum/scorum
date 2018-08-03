#pragma once
#include <scorum/app/plugin.hpp>

#include <scorum/blockchain_monitoring/schema/bucket_object.hpp>

#ifndef BLOCKCHAIN_MONITORING_PLUGIN_NAME
#define BLOCKCHAIN_MONITORING_PLUGIN_NAME "blockchain_monitoring"
#endif

namespace scorum {
namespace blockchain_monitoring {

using app::application;

namespace detail {
class blockchain_monitoring_plugin_impl;
}

/**
 * @brief This plugin designed to track statiscs over the time window
 *
 * @ingroup plugins
 * @defgroup blockchain_monitoring_plugin Blockchain monitoring plugin
 */
class blockchain_monitoring_plugin : public scorum::app::plugin
{
public:
    blockchain_monitoring_plugin(application* app);
    virtual ~blockchain_monitoring_plugin();

    virtual std::string plugin_name() const override
    {
        return BLOCKCHAIN_MONITORING_PLUGIN_NAME;
    }
    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;

    const flat_set<uint32_t>& get_tracked_buckets() const;
    uint32_t get_max_history_per_bucket() const;

    uint32_t get_last_block_duration_microseconds() const;

private:
    friend class detail::blockchain_monitoring_plugin_impl;
    std::unique_ptr<detail::blockchain_monitoring_plugin_impl> _my;
};

} // namespace blockchain_monitoring
} // namespace scorum
