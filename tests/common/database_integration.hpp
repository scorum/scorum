#pragma once

#include <scorum/app/application.hpp>
#include <scorum/chain/database.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <scorum/plugins/debug_node/debug_node_plugin.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <iostream>

#include "defines.hpp"
#include "genesis.hpp"

namespace scorum {
namespace chain {

using namespace scorum::protocol;

class database_integration_fixture
{
public:
    database_integration_fixture();
    virtual ~database_integration_fixture();

    static Genesis default_genesis_state();
    static genesis_state_type create_default_genesis_state();

    void open_database(const genesis_state_type& genesis);
    void open_database()
    {
        open_database(genesis_state);
    }

    void validate_database();

    void generate_block(uint32_t skip = 0,
                        const private_key_type& key = generate_private_key(TEST_INIT_KEY),
                        int miss_blocks = 0);

    void generate_blocks(uint32_t block_count);

    void generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks = true);

    static private_key_type generate_private_key(const std::string& seed);

protected:
    virtual void open_database_impl(const genesis_state_type& genesis);

private:
    bool opened = false;

public:
    scorum::app::application app;
    chain::database& db;
    genesis_state_type genesis_state;

    const private_key_type init_account_priv_key;
    const public_key_type init_account_pub_key;

    const std::string debug_key;
    const uint32_t default_skip;

    std::shared_ptr<scorum::plugin::debug_node::debug_node_plugin> db_plugin;

    optional<fc::temp_directory> data_dir;
};
}
}
