
#include <scorum/app/application.hpp>
#include <scorum/app/plugin.hpp>
#include <scorum/plugins/debug_node/debug_node_api.hpp>
#include <scorum/plugins/debug_node/debug_node_plugin.hpp>
#include <scorum/chain/services/witness.hpp>

#include <fc/io/buffered_iostream.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>

#include <fc/thread/future.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <sstream>
#include <string>

namespace scorum {
namespace plugin {
namespace debug_node {

debug_node_plugin::debug_node_plugin(application* app)
    : plugin(app)
{
}
debug_node_plugin::~debug_node_plugin()
{
}

std::string debug_node_plugin::plugin_name() const
{
    return "debug_node";
}

void debug_node_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                   boost::program_options::options_description& cfg)
{
    // cli.add_options()("edit-script,e", boost::program_options::value<std::vector<std::string>>()->composing(),
    //                  "Database edits to apply on startup (may specify multiple times)");
    cfg.add(cli);
}

void debug_node_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
}

void debug_node_plugin::plugin_startup()
{
    if (logging)
        ilog("debug_node_plugin::plugin_startup() begin");
    chain::database& db = database();

    // connect needed signals
    _applied_block_conn = db.applied_block.connect([this](const chain::signed_block& b) { on_applied_block(b); });

    app().register_api_factory<debug_node_api>("debug_node_api");
}

uint32_t debug_node_plugin::debug_generate_blocks(
    const std::string& debug_key, uint32_t count, uint32_t skip, uint32_t miss_blocks, private_key_storage* key_storage)
{
    if (count == 0)
        return 0;

    scorum::chain::database& db = database();

    auto& witness_service = db.obtain_service<chain::dbs_witness>();

    fc::optional<fc::ecc::private_key> debug_private_key;
    scorum::chain::public_key_type debug_public_key;
    if (debug_key != "")
    {
        debug_private_key = graphene::utilities::wif_to_key(debug_key);
        FC_ASSERT(debug_private_key.valid());
        debug_public_key = debug_private_key->get_public_key();
    }

    uint32_t slot = miss_blocks + 1, produced = 0;
    while (produced < count)
    {
        uint32_t new_slot = miss_blocks + 1;
        std::string scheduled_witness_name = db.get_scheduled_witness(slot);
        fc::time_point_sec scheduled_time = db.get_slot_time(slot);
        const chain::witness_object& scheduled_witness = witness_service.get(scheduled_witness_name);
        scorum::chain::public_key_type scheduled_key = scheduled_witness.signing_key;
        if (debug_key != "")
        {
            if (logging)
                wlog("scheduled key is: ${sk}   dbg key is: ${dk}", ("sk", scheduled_key)("dk", debug_public_key));
            if (scheduled_key != debug_public_key)
            {
                if (logging)
                    wlog("Modified key for witness ${w}", ("w", scheduled_witness_name));
                debug_update(
                    [=](chain::database& db) {
                        db.modify(scheduled_witness,
                                  [&](chain::witness_object& w) { w.signing_key = debug_public_key; });
                    },
                    skip);
            }
        }
        else
        {
            debug_private_key.reset();
            if (key_storage != nullptr)
                key_storage->maybe_get_private_key(debug_private_key, scheduled_key, scheduled_witness_name);
            if (!debug_private_key.valid())
            {
                if (logging)
                    elog("Skipping ${wit} because I don't know the private key", ("wit", scheduled_witness_name));
                new_slot = slot + 1;
                FC_ASSERT(slot < miss_blocks + 50);
            }
        }
        db.generate_block(scheduled_time, scheduled_witness_name, *debug_private_key, skip);
        ++produced;
        slot = new_slot;
    }

    return count;
}

uint32_t debug_node_plugin::debug_generate_blocks_until(const std::string& debug_key,
                                                        const fc::time_point_sec& head_block_time,
                                                        bool generate_sparsely,
                                                        uint32_t skip,
                                                        private_key_storage* key_storage)
{
    scorum::chain::database& db = database();

    if (db.head_block_time() >= head_block_time)
        return 0;

    uint32_t new_blocks = 0;

    if (generate_sparsely)
    {
        new_blocks += debug_generate_blocks(debug_key, 1, skip);
        auto slots_to_miss = db.get_slot_at_time(head_block_time);
        if (slots_to_miss > 1)
        {
            slots_to_miss--;
            new_blocks += debug_generate_blocks(debug_key, 1, skip, slots_to_miss, key_storage);
        }
    }
    else
    {
        while (db.head_block_time() < head_block_time)
            new_blocks += debug_generate_blocks(debug_key, 1);
    }

    return new_blocks;
}

void debug_node_plugin::apply_debug_updates()
{
    // this was a method on database in Graphene
    chain::database& db = database();
    chain::block_id_type head_id = db.head_block_id();
    auto it = _debug_updates.find(head_id);
    if (it == _debug_updates.end())
        return;

    for (auto& update : it->second)
        update(db);
}

void debug_node_plugin::on_applied_block(const chain::signed_block& b)
{
    try
    {
        if (!_debug_updates.empty())
            apply_debug_updates();
    }
    FC_LOG_AND_RETHROW()
}

void debug_node_plugin::plugin_shutdown()
{
}

} // namespace debug_node
} // namespace plugin
} // namespace scorum

SCORUM_DEFINE_PLUGIN(debug_node, scorum::plugin::debug_node::debug_node_plugin)
