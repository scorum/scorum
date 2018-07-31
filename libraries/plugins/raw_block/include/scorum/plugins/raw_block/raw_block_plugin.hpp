
#pragma once

#include <scorum/app/plugin.hpp>

namespace scorum {
namespace plugin {
namespace raw_block {

using scorum::app::application;

/**
 * @ingroup plugins
 * @defgroup raw_block_plugin Raw block plugin
 */
class raw_block_plugin : public scorum::app::plugin
{
public:
    raw_block_plugin(application* app);
    virtual ~raw_block_plugin();

    virtual std::string plugin_name() const override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;
    virtual void plugin_shutdown() override;
};
}
}
}
