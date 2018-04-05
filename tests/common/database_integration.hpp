#pragma once

#include <scorum/app/application.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <scorum/plugins/debug_node/debug_node_plugin.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <iostream>

#include "defines.hpp"
#include "genesis.hpp"

namespace database_fixture {

using namespace scorum::chain;
using namespace scorum::protocol;

class database_integration_fixture
{
public:
    database_integration_fixture();
    virtual ~database_integration_fixture();

    static Genesis default_genesis_state();
    static genesis_state_type create_default_genesis_state();
    static private_key_type generate_private_key(const std::string& seed);

    void open_database(const genesis_state_type& genesis);
    void open_database();

    void validate_database();

    void generate_block(uint32_t skip = 0, const private_key_type& key = initdelegate.private_key, int miss_blocks = 0);

    void generate_blocks(uint32_t block_count);

    void generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks = true);

protected:
    virtual void open_database_impl(const genesis_state_type& genesis);

    template <class Plugin> std::shared_ptr<Plugin> init_plugin()
    {
        boost::program_options::variables_map options;

        auto plugin = app.register_plugin<Plugin>();
        app.enable_plugin(plugin->plugin_name());
        plugin->plugin_initialize(options);
        plugin->plugin_startup();

        return plugin;
    }

private:
    bool opened = false;

public:
    static Actor initdelegate;

    scorum::app::application app;
    scorum::chain::database& db;

    genesis_state_type genesis_state;

    const std::string debug_key;
    const uint32_t default_skip;

    std::shared_ptr<scorum::plugin::debug_node::debug_node_plugin> db_plugin;

    fc::optional<fc::temp_directory> data_dir;
};

} // database_fixture
