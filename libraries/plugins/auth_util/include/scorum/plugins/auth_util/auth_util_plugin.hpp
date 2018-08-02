
#pragma once

#include <scorum/app/plugin.hpp>

namespace scorum {
namespace plugin {
namespace auth_util {

using scorum::app::application;

/**
 * @brief The auth_util_plugin class
 *
 * @ingroup plugins
 * @addtogroup auth_util_plugin Auth util plugin
 */
class auth_util_plugin : public scorum::app::plugin
{
public:
    auth_util_plugin(application* app);
    virtual ~auth_util_plugin();

    virtual std::string plugin_name() const override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;
    virtual void plugin_shutdown() override;
};
}
}
}
