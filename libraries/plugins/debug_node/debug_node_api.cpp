
#include <fc/filesystem.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <scorum/app/api_context.hpp>
#include <scorum/app/application.hpp>

#include <scorum/protocol/block.hpp>

#include <scorum/chain/block_log.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <scorum/plugins/debug_node/debug_node_api.hpp>
#include <scorum/plugins/debug_node/debug_node_plugin.hpp>

namespace scorum {
namespace plugin {
namespace debug_node {

namespace detail {

class debug_private_key_storage : public private_key_storage
{
public:
    debug_private_key_storage()
    {
    }
    virtual ~debug_private_key_storage()
    {
    }

    virtual void maybe_get_private_key(fc::optional<fc::ecc::private_key>& result,
                                       const scorum::chain::public_key_type& pubkey,
                                       const std::string& account_name) override;

    std::string dev_key_prefix;
    std::map<scorum::chain::public_key_type, fc::ecc::private_key> key_table;
};

class debug_node_api_impl
{
public:
    debug_node_api_impl(scorum::app::application& _app);

    uint32_t debug_push_blocks(const std::string& src_filename, uint32_t count, bool skip_validate_invariants);
    uint32_t debug_generate_blocks(const std::string& debug_key, uint32_t count);
    uint32_t debug_generate_blocks_until(const std::string& debug_key,
                                         const fc::time_point_sec& head_block_time,
                                         bool generate_sparsely);
    fc::optional<scorum::chain::signed_block> debug_pop_block();
    // void debug_push_block( const scorum::chain::signed_block& block );
    scorum::chain::witness_schedule_object debug_get_witness_schedule();
    scorum::chain::hardfork_property_object debug_get_hardfork_property_object();
    void debug_update_object(const fc::variant_object& update);
    fc::variant_object debug_get_edits();
    void debug_set_edits(const fc::variant_object& edits);
    // void debug_save_db( std::string db_path );
    void debug_stream_json_objects(const std::string& filename);
    void debug_stream_json_objects_flush();
    void debug_set_hardfork(uint32_t hardfork_id);
    bool debug_has_hardfork(uint32_t hardfork_id);
    void debug_get_json_schema(std::string& schema);
    void debug_set_dev_key_prefix(std::string prefix);
    void debug_get_dev_key(get_dev_key_result& result, const get_dev_key_args& args);
    std::shared_ptr<scorum::plugin::debug_node::debug_node_plugin> get_plugin();

    scorum::app::application& app;
    debug_private_key_storage key_storage;
};

void debug_private_key_storage::maybe_get_private_key(fc::optional<fc::ecc::private_key>& result,
                                                      const scorum::chain::public_key_type& pubkey,
                                                      const std::string& account_name)
{
    auto it = key_table.find(pubkey);
    if (it != key_table.end())
    {
        result = it->second;
        return;
    }
    fc::ecc::private_key gen_priv = fc::ecc::private_key::regenerate(fc::sha256::hash(dev_key_prefix + account_name));
    chain::public_key_type gen_pub = gen_priv.get_public_key();
    key_table[gen_pub] = gen_priv;
    if ((pubkey == scorum::chain::public_key_type()) || (gen_pub == pubkey))
    {
        result = gen_priv;
        return;
    }

    result.reset();
    return;
}

debug_node_api_impl::debug_node_api_impl(scorum::app::application& _app)
    : app(_app)
{
#ifdef SCORUM_INIT_PRIVATE_KEY
    fc::ecc::private_key init_key = SCORUM_INIT_PRIVATE_KEY;
    key_storage.key_table[init_key.get_public_key()] = init_key;
#endif
}

void debug_node_api_impl::debug_set_dev_key_prefix(std::string prefix)
{
    key_storage.dev_key_prefix = prefix;
    return;
}

void debug_node_api_impl::debug_get_dev_key(get_dev_key_result& result, const get_dev_key_args& args)
{
    fc::ecc::private_key priv
        = fc::ecc::private_key::regenerate(fc::sha256::hash(key_storage.dev_key_prefix + args.name));
    result.private_key = graphene::utilities::key_to_wif(priv);
    result.public_key = priv.get_public_key();
    return;
}

uint32_t
debug_node_api_impl::debug_push_blocks(const std::string& src_filename, uint32_t count, bool skip_validate_invariants)
{
    if (count == 0)
        return 0;

    std::shared_ptr<scorum::chain::database> db = app.chain_database();
    fc::path src_path = fc::path(src_filename);
    fc::path index_path = fc::path(src_filename + ".index");
    if (fc::exists(src_path) && fc::exists(index_path) && !fc::is_directory(src_path) && !fc::is_directory(index_path))
    {
        ilog("Loading ${n} from block_log ${fn}", ("n", count)("fn", src_filename));
        idump((src_filename)(count)(skip_validate_invariants));
        scorum::chain::block_log log;
        log.open(src_path);
        uint32_t first_block = db->head_block_num() + 1;
        uint32_t skip_flags = scorum::chain::database::skip_nothing;
        if (skip_validate_invariants)
            skip_flags = skip_flags | scorum::chain::database::skip_validate_invariants;
        for (uint32_t i = 0; i < count; i++)
        {
            // fc::optional< scorum::chain::signed_block > block = log.read_block( log.get_block_pos( first_block + i )
            // );
            uint64_t block_pos = log.get_block_pos(first_block + i);
            if (block_pos == scorum::chain::block_log::npos)
            {
                wlog("Block database ${fn} only contained ${i} of ${n} requested blocks",
                     ("i", i)("n", count)("fn", src_filename));
                return i;
            }

            decltype(log.read_block(0)) result;

            try
            {
                result = log.read_block(block_pos);
            }
            catch (const fc::exception& e)
            {
                elog("Could not read block ${i} of ${n}", ("i", i)("n", count));
                continue;
            }

            try
            {
                db->push_block(result.first, skip_flags);
            }
            catch (const fc::exception& e)
            {
                elog("Got exception pushing block ${bn} : ${bid} (${i} of ${n})",
                     ("bn", result.first.block_num())("bid", result.first.id())("i", i)("n", count));
                elog("Exception backtrace: ${bt}", ("bt", e.to_detail_string()));
            }
        }
        ilog("Completed loading block_database successfully");
        return count;
    }
    return 0;
}

uint32_t debug_node_api_impl::debug_generate_blocks(const std::string& debug_key, uint32_t count)
{
    return get_plugin()->debug_generate_blocks(debug_key, count, scorum::chain::database::skip_nothing, 0,
                                               &key_storage);
}

uint32_t debug_node_api_impl::debug_generate_blocks_until(const std::string& debug_key,
                                                          const fc::time_point_sec& head_block_time,
                                                          bool generate_sparsely)
{
    return get_plugin()->debug_generate_blocks_until(debug_key, head_block_time, generate_sparsely,
                                                     scorum::chain::database::skip_nothing, &key_storage);
}

fc::optional<scorum::chain::signed_block> debug_node_api_impl::debug_pop_block()
{
    std::shared_ptr<scorum::chain::database> db = app.chain_database();
    return db->fetch_block_by_number(db->head_block_num());
}

/*void debug_node_api_impl::debug_push_block( const scorum::chain::signed_block& block )
{
   app.chain_database()->push_block( block );
}*/

scorum::chain::witness_schedule_object debug_node_api_impl::debug_get_witness_schedule()
{
    return app.chain_database()->get(scorum::chain::witness_schedule_id_type());
}

scorum::chain::hardfork_property_object debug_node_api_impl::debug_get_hardfork_property_object()
{
    return app.chain_database()->get(scorum::chain::hardfork_property_id_type());
}

void debug_node_api_impl::debug_update_object(const fc::variant_object& update)
{
    // get_plugin()->debug_update( update );
}

/*fc::variant_object debug_node_api_impl::debug_get_edits()
{
   fc::mutable_variant_object result;
   get_plugin()->save_debug_updates( result );
   return fc::variant_object( std::move( result ) );
}*/

void debug_node_api_impl::debug_set_edits(const fc::variant_object& edits)
{
    // get_plugin()->load_debug_updates( edits );
}

std::shared_ptr<scorum::plugin::debug_node::debug_node_plugin> debug_node_api_impl::get_plugin()
{
    return app.get_plugin<debug_node_plugin>("debug_node");
}

void debug_node_api_impl::debug_stream_json_objects(const std::string& filename)
{
    // get_plugin()->set_json_object_stream( filename );
}

void debug_node_api_impl::debug_stream_json_objects_flush()
{
    // get_plugin()->flush_json_object_stream();
}

void debug_node_api_impl::debug_set_hardfork(uint32_t hardfork_id)
{
    using namespace scorum::chain;

    if (hardfork_id > SCORUM_NUM_HARDFORKS)
        return;

    get_plugin()->debug_update([=](database& db) { db.set_hardfork(hardfork_id, false); });
}

bool debug_node_api_impl::debug_has_hardfork(uint32_t hardfork_id)
{
    return app.chain_database()->get(scorum::chain::hardfork_property_id_type()).last_hardfork >= hardfork_id;
}

} // detail

debug_node_api::debug_node_api(const scorum::app::api_context& ctx)
{
    my = std::make_shared<detail::debug_node_api_impl>(ctx.app);
}

void debug_node_api::on_api_startup()
{
}

uint32_t debug_node_api::debug_push_blocks(std::string source_filename, uint32_t count, bool skip_validate_invariants)
{
    return my->debug_push_blocks(source_filename, count, skip_validate_invariants);
}

uint32_t debug_node_api::debug_generate_blocks(std::string debug_key, uint32_t count)
{
    return my->debug_generate_blocks(debug_key, count);
}

uint32_t debug_node_api::debug_generate_blocks_until(std::string debug_key,
                                                     fc::time_point_sec head_block_time,
                                                     bool generate_sparsely)
{
    return my->debug_generate_blocks_until(debug_key, head_block_time, generate_sparsely);
}

fc::optional<scorum::chain::signed_block> debug_node_api::debug_pop_block()
{
    return my->debug_pop_block();
}

/*void debug_node_api::debug_push_block( scorum::chain::signed_block& block )
{
   my->debug_push_block( block );
}*/

scorum::chain::witness_schedule_object debug_node_api::debug_get_witness_schedule()
{
    return my->debug_get_witness_schedule();
}

scorum::chain::hardfork_property_object debug_node_api::debug_get_hardfork_property_object()
{
    return my->debug_get_hardfork_property_object();
}

/*
void debug_node_api::debug_update_object( fc::variant_object update )
{
   my->debug_update_object( update );
}
*/

/*
fc::variant_object debug_node_api::debug_get_edits()
{
   return my->debug_get_edits();
}
*/

/*
void debug_node_api::debug_set_edits( fc::variant_object edits )
{
   my->debug_set_edits(edits);
}
*/

void debug_node_api::debug_set_dev_key_prefix(std::string prefix)
{
    my->debug_set_dev_key_prefix(prefix);
}

get_dev_key_result debug_node_api::debug_get_dev_key(get_dev_key_args args)
{
    get_dev_key_result result;
    my->debug_get_dev_key(result, args);
    return result;
}

/*
void debug_node_api::debug_stream_json_objects( std::string filename )
{
   my->debug_stream_json_objects( filename );
}

void debug_node_api::debug_stream_json_objects_flush()
{
   my->debug_stream_json_objects_flush();
}
*/

void debug_node_api::debug_set_hardfork(uint32_t hardfork_id)
{
    my->debug_set_hardfork(hardfork_id);
}

bool debug_node_api::debug_has_hardfork(uint32_t hardfork_id)
{
    return my->debug_has_hardfork(hardfork_id);
}
}
}
} // scorum::plugin::debug_node
