#pragma once

#include <scorum/app/application.hpp>
#include <scorum/chain/database.hpp>
#include <scorum/chain/genesis_state.hpp>
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

void create_initdelegate_for_genesis_state(genesis_state_type& genesis_state);

struct database_fixture
{
    // the reason we use an app is to exercise the indexes of built-in
    //   plugins
    scorum::app::application app;
    chain::database& db;
    genesis_state_type genesis_state;
    signed_transaction trx;
    public_key_type committee_key;
    account_id_type committee_account;

    const private_key_type init_account_priv_key;
    const public_key_type init_account_pub_key;

    const std::string debug_key;
    const uint32_t default_skip;

    std::shared_ptr<scorum::plugin::debug_node::debug_node_plugin> db_plugin;

    optional<fc::temp_directory> data_dir;

    database_fixture();
    ~database_fixture();

    static private_key_type generate_private_key(const std::string& seed);
    void open_database();

    void generate_block(uint32_t skip = 0,
                        const private_key_type& key = generate_private_key("init_key"),
                        int miss_blocks = 0);

    /**
     * @brief Generates block_count blocks
     * @param block_count number of blocks to generate
     */
    void generate_blocks(uint32_t block_count);

    /**
     * @brief Generates blocks until the head block time matches or exceeds timestamp
     * @param timestamp target time to generate blocks until
     */
    void generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks = true);

    const account_object& account_create(const string& name,
                                         const string& creator,
                                         const private_key_type& creator_key,
                                         const share_type& fee,
                                         const public_key_type& key,
                                         const public_key_type& post_key,
                                         const string& json_metadata);

    const account_object&
    account_create(const string& name, const public_key_type& key, const public_key_type& post_key);

    const account_object& account_create(const string& name, const public_key_type& key);

    const witness_object& witness_create(const string& owner,
                                         const private_key_type& owner_key,
                                         const string& url,
                                         const public_key_type& signing_key,
                                         const share_type& fee);

    void fund(const string& account_name, const share_type& amount = 500000);
    void fund(const string& account_name, const asset& amount);
    void transfer(const string& from, const string& to, const share_type& scorum);
    void convert(const string& account_name, const asset& amount);
    void vest(const string& from, const share_type& amount);
    void vest(const string& account, const asset& amount);
    void proxy(const string& account, const string& proxy);
    const asset& get_balance(const string& account_name) const;
    void sign(signed_transaction& trx, const fc::ecc::private_key& key);

    vector<operation> get_last_operations(uint32_t ops);

    void validate_database(void);
};

struct clean_database_fixture : public database_fixture
{
    clean_database_fixture();
    ~clean_database_fixture();

    void resize_shared_mem(uint64_t size);
};

struct live_database_fixture : public database_fixture
{
    live_database_fixture();
    ~live_database_fixture();

    fc::path _chain_dir;
};

struct timed_blocks_database_fixture : public clean_database_fixture
{
    timed_blocks_database_fixture();

    fc::time_point_sec default_deadline;
    const int BLOCK_LIMIT_DEFAULT = 5;

private:
    static bool _time_printed;
};

namespace test {
bool _push_block(database& db, const signed_block& b, uint32_t skip_flags = 0);
void _push_transaction(database& db, const signed_transaction& tx, uint32_t skip_flags = 0);
} // namespace test
} // namespace chain
} // namespace scorum
