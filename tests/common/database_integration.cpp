#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>
#include <scorum/witness/witness_plugin.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/services/account.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_integration.hpp"

Actor database_fixture::database_integration_fixture::initdelegate = Actor(TEST_INIT_DELEGATE_NAME);

namespace database_fixture {

database_integration_fixture::database_integration_fixture()
    : app(std::make_shared<database>(database::opt_notify_virtual_op_applying))
    , db(*app.chain_database())
    , debug_key(graphene::utilities::key_to_wif(initdelegate.private_key))
    , default_skip(0 | database::skip_undo_history_check | database::skip_authority_check | database::skip_tapos_check)
{
    genesis_state = create_default_genesis_state();
}

database_integration_fixture::~database_integration_fixture()
{
}

Genesis database_integration_fixture::default_genesis_state()
{
    static Genesis default_genesis;

    if (default_genesis._accounts.empty())
    {
        initdelegate.scorum(TEST_ACCOUNTS_INITIAL_SUPPLY);

        default_genesis = Genesis::create()
                              .accounts_supply(TEST_ACCOUNTS_INITIAL_SUPPLY)
                              .rewards_supply(TEST_REWARD_INITIAL_SUPPLY)
                              .dev_committee(initdelegate)
                              .accounts(initdelegate)
                              .witnesses(initdelegate);
    }

    return default_genesis;
}

genesis_state_type database_integration_fixture::create_default_genesis_state()
{
    return default_genesis_state().generate();
}

void database_integration_fixture::open_database(const genesis_state_type& genesis)
{
    FC_ASSERT(!opened);

    try
    {
        opened = true;

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

        db_plugin = app.register_plugin<scorum::plugin::debug_node::debug_node_plugin>();
        auto wit_plugin = app.register_plugin<scorum::witness::witness_plugin>();

        boost::program_options::variables_map options;

        db_plugin->logging = false;
        db_plugin->plugin_initialize(options);
        wit_plugin->plugin_initialize(options);

        open_database_impl(genesis);

        db_plugin->plugin_startup();

        validate_database();
    }
    catch (const fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

void database_integration_fixture::open_database()
{
    open_database(genesis_state);
}

void database_integration_fixture::validate_database()
{
    try
    {
        db.validate_invariants();
    }
    FC_LOG_AND_RETHROW();
}

void database_integration_fixture::generate_block(uint32_t skip, const fc::ecc::private_key& key, int miss_blocks)
{
    skip |= default_skip;
    db_plugin->debug_generate_blocks(graphene::utilities::key_to_wif(key), 1, skip, miss_blocks);
}

void database_integration_fixture::generate_blocks(uint32_t block_count)
{
    auto produced = db_plugin->debug_generate_blocks(debug_key, block_count, default_skip, 0);
    BOOST_REQUIRE(produced == block_count);
}

void database_integration_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks)
{
    db_plugin->debug_generate_blocks_until(debug_key, timestamp, miss_intermediate_blocks, default_skip);
    BOOST_REQUIRE((db.head_block_time() - timestamp).to_seconds() < SCORUM_BLOCK_INTERVAL);
}

private_key_type database_integration_fixture::generate_private_key(const std::string& seed)
{
    static const private_key_type committee
        = private_key_type::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
    if (seed == TEST_INIT_KEY)
        return committee;
    return fc::ecc::private_key::regenerate(fc::sha256::hash(seed));
}
void database_integration_fixture::open_database_impl(const genesis_state_type& genesis)
{
    if (!data_dir)
    {
        data_dir = fc::temp_directory(graphene::utilities::temp_directory_path());
        db.open(data_dir->path(), data_dir->path(), TEST_SHARED_MEM_SIZE_10MB, chainbase::database::read_write,
                genesis);
        genesis_state = genesis;
    }
}

} // database_fixture
