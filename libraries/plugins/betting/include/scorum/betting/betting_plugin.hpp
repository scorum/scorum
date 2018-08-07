#pragma once
#include <scorum/app/plugin.hpp>

#ifndef BETTING_PLUGIN_NAME
#define BETTING_PLUGIN_NAME "betting"
#endif

namespace scorum {
namespace betting {

class betting_plugin : public scorum::app::plugin
{
public:
    betting_plugin(app::application* app);
    virtual ~betting_plugin();

    virtual std::string plugin_name() const override;
    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;
};
}
}
