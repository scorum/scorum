#include <scorum/betting/betting_plugin.hpp>
#include <scorum/betting/betting_api.hpp>

namespace scorum {
namespace betting {

betting_plugin::betting_plugin(app::application* app)
    : plugin(app)
{
}

betting_plugin::~betting_plugin() = default;

std::string betting_plugin::plugin_name() const
{
    return BETTING_PLUGIN_NAME;
}

void betting_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                boost::program_options::options_description& cfg)
{
    // do nothing.
}
void betting_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    print_greeting();
}

void betting_plugin::plugin_startup()
{
    app().register_api_factory<betting_api>(API_BETTING);
}
}
}

SCORUM_DEFINE_PLUGIN(betting, scorum::betting::betting_plugin)
