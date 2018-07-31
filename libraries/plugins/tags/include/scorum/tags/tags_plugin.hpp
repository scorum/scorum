#pragma once

#include <scorum/app/plugin.hpp>

namespace scorum {
namespace tags {

namespace detail {
class tags_plugin_impl;
}

using namespace scorum::chain;

/**
 * @brief This plugin will scan all changes to posts and/or their meta data and
 *
 * @ingroup plugins
 * @defgroup tags_plugin Tags plugin
 */
class tags_plugin : public scorum::app::plugin
{
public:
    tags_plugin(scorum::app::application* app);
    virtual ~tags_plugin();

    std::string plugin_name() const override;
    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;

    friend class detail::tags_plugin_impl;
    std::unique_ptr<detail::tags_plugin_impl> my;
};

} // namespace tags
} // namespace scorum
