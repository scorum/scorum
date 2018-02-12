#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/history_objects.hpp>
#include <scorum/account_history/account_history_plugin.hpp>
#include <scorum/witness/witness_plugin.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/services/account.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "genesis_db_fixture.hpp"

namespace scorum {
namespace chain {

genesis_db_fixture::genesis_db_fixture()
    : app()
    , db(*app.chain_database())
    , init_account_priv_key(private_key_type::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY))))
    , init_account_pub_key(init_account_priv_key.get_public_key())
    , debug_key(graphene::utilities::key_to_wif(init_account_priv_key))
    , default_skip(0 | database::skip_undo_history_check | database::skip_authority_check)
{
}

genesis_db_fixture::~genesis_db_fixture()
{
}

void genesis_db_fixture::apply_genesis(const genesis_state_type& genesis)
{
    if (single_apply)
        return;
    try
    {
        single_apply = true;

        int argc = boost::unit_test::framework::master_test_suite().argc;
        char** argv = boost::unit_test::framework::master_test_suite().argv;
        for (int i = 1; i < argc; i++)
        {
            const std::string arg = argv[i];
            if (arg == "--record-assert-trip")
                fc::enable_record_assert_trip = true;
            if (arg == "--show-test-names")
                std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
        }
        auto ahplugin = app.register_plugin<scorum::account_history::account_history_plugin>();
        db_plugin = app.register_plugin<scorum::plugin::debug_node::debug_node_plugin>();
        auto wit_plugin = app.register_plugin<scorum::witness::witness_plugin>();

        boost::program_options::variables_map options;

        db_plugin->logging = false;
        ahplugin->plugin_initialize(options);
        db_plugin->plugin_initialize(options);
        wit_plugin->plugin_initialize(options);

        open_database(genesis);

        generate_block();
        db.set_hardfork(SCORUM_NUM_HARDFORKS);
        generate_block();

        db_plugin->plugin_startup();

        validate_database();
    }
    catch (const fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

private_key_type genesis_db_fixture::generate_private_key(const std::string& seed)
{
    static const private_key_type committee
        = private_key_type::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
    if (seed == TEST_INIT_KEY)
        return committee;
    return fc::ecc::private_key::regenerate(fc::sha256::hash(seed));
}

void genesis_db_fixture::open_database(const genesis_state_type& genesis)
{
    if (!data_dir)
    {
        data_dir = fc::temp_directory(graphene::utilities::temp_directory_path());
        db._log_hardforks = false;
        db.open(data_dir->path(), data_dir->path(), TEST_SHARED_MEM_SIZE_8MB, chainbase::database::read_write, genesis);
    }
}

void genesis_db_fixture::generate_blocks(uint32_t block_count)
{
    auto produced = db_plugin->debug_generate_blocks(debug_key, block_count, default_skip, 0);
    BOOST_REQUIRE_EQUAL(produced, block_count);
}

void genesis_db_fixture::validate_database(void)
{
    try
    {
        db.validate_invariants();
    }
    FC_LOG_AND_RETHROW();
}

} // namespace chain
} // namespace scorum
