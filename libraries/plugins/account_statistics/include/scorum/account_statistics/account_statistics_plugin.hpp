#pragma once

#include <scorum/app/plugin.hpp>

#include <scorum/account_statistics/schema/objects.hpp>

#ifndef ACCOUNT_STATISTICS_PLUGIN_NAME
#define ACCOUNT_STATISTICS_PLUGIN_NAME "account_statistics"
#endif

namespace scorum {
namespace account_statistics {

using app::application;

namespace detail {
class account_statistics_plugin_impl;
}

/**
 * @brief This plugin designed to track statistics for over the time window for each account
 *
 * @ingroup plugins
 * @addtogroup account_statistics_plugin Account statistics plugin
 */
class account_statistics_plugin : public scorum::app::plugin
{
public:
    account_statistics_plugin(application* app);
    virtual ~account_statistics_plugin();

    virtual std::string plugin_name() const override
    {
        return ACCOUNT_STATISTICS_PLUGIN_NAME;
    }
    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;

    const flat_set<uint32_t>& get_tracked_buckets() const;
    uint32_t get_max_history_per_bucket() const;

private:
    friend class detail::account_statistics_plugin_impl;
    std::unique_ptr<detail::account_statistics_plugin_impl> _my;
};
}
} // scorum::account_statistics
