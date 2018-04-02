#pragma once
#include <scorum/app/plugin.hpp>

#include <scorum/blockchain_statistics/schema/bucket_object.hpp>

#ifndef BLOCKCHAIN_STATISTICS_PLUGIN_NAME
#define BLOCKCHAIN_STATISTICS_PLUGIN_NAME "chain_stats"
#endif

namespace scorum {
namespace blockchain_statistics {

using app::application;

namespace detail {
class blockchain_statistics_plugin_impl;
}

class blockchain_statistics_plugin : public scorum::app::plugin
{
public:
    blockchain_statistics_plugin(application* app);
    virtual ~blockchain_statistics_plugin();

    virtual std::string plugin_name() const override
    {
        return BLOCKCHAIN_STATISTICS_PLUGIN_NAME;
    }
    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;

    const flat_set<uint32_t>& get_tracked_buckets() const;
    uint32_t get_max_history_per_bucket() const;

    // node monitoring API
    uint32_t get_last_block_duration_microseconds() const;

private:
    friend class detail::blockchain_statistics_plugin_impl;
    std::unique_ptr<detail::blockchain_statistics_plugin_impl> _my;
};

} // namespace blockchain_statistics
} // namespace scorum
