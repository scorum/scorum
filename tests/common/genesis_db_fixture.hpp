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

using namespace graphene::db;

namespace scorum {
namespace chain {

using namespace scorum::protocol;

struct genesis_db_fixture
{
    genesis_db_fixture();
    ~genesis_db_fixture();

    void apply_genesis(const genesis_state_type&);

private:
    static private_key_type generate_private_key(const std::string& seed);
    void open_database(const genesis_state_type& genesis);

    void generate_block()
    {
        generate_blocks(0);
    }
    void generate_blocks(uint32_t block_count);

    void validate_database(void);

protected:
    bool single_apply = false;
    scorum::app::application app;
    chain::database& db;

    const private_key_type init_account_priv_key;
    const public_key_type init_account_pub_key;

    const std::string debug_key;
    const uint32_t default_skip;

    std::shared_ptr<scorum::plugin::debug_node::debug_node_plugin> db_plugin;

    optional<fc::temp_directory> data_dir;
};

} // namespace chain
} // namespace scorum
