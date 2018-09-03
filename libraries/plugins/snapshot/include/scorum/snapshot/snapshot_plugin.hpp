#pragma once

#include <scorum/app/plugin.hpp>

#ifndef SNAPSHOT_PLUGIN_NAME
#define SNAPSHOT_PLUGIN_NAME "snapshot"
#endif

namespace scorum {
namespace snapshot {

namespace detail {
class snapshot_plugin_impl;
}

using app::application;

/**
 * @brief This plugin responsible for snaphot saving and loading
 *
 * @ingroup plugins
 * @defgroup snapshot_plugin Snapshot plugin
 */
class snapshot_plugin : public scorum::app::plugin
{
public:
    snapshot_plugin(application* app);
    virtual ~snapshot_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(boost::program_options::options_description& command_line_options,
                                            boost::program_options::options_description& config_file_options) override;

    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup()
    {
    }
    virtual void plugin_shutdown()
    {
    }

    uint32_t get_snapshot_number() const;

private:
    std::unique_ptr<detail::snapshot_plugin_impl> _impl;
};
}
} // scorum::snapshot
