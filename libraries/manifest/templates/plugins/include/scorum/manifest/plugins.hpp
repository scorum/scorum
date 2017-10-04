
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace scorum { namespace app {

class abstract_plugin;
class application;

} }

namespace scorum { namespace plugin {

void initialize_plugin_factories();
std::shared_ptr< scorum::app::abstract_plugin > create_plugin( const std::string& name, scorum::app::application* app );
std::vector< std::string > get_available_plugins();

} }
