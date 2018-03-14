/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <scorum/witness/witness_objects.hpp>
#include <scorum/witness/witness_plugin.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/database_exceptions.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <fc/time.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>
#include <memory>

#define DISTANCE_CALC_PRECISION (10000)

namespace scorum {
namespace witness {

namespace bpo = boost::program_options;

using protocol::signed_transaction;
using chain::account_object;

namespace detail {
using namespace scorum::chain;

class witness_plugin_impl
{
public:
    witness_plugin_impl(witness_plugin& plugin)
        : _self(plugin)
    {
    }

    void plugin_initialize();

    void pre_transaction(const signed_transaction& trx);
    void pre_operation(const operation_notification& note);
    void on_block(const signed_block& b);

    void update_account_bandwidth(const account_object& a, uint32_t trx_size, const bandwidth_type type);

    witness_plugin& _self;
};

void witness_plugin_impl::plugin_initialize()
{
}

void check_memo(const std::string& memo, const account_object& account, const account_authority_object& auth)
{
    std::vector<public_key_type> keys;

    try
    {
        // Check if memo is a private key
        keys.push_back(fc::ecc::extended_private_key::from_base58(memo).get_public_key());
    }
    catch (fc::parse_error_exception&)
    {
    }
    catch (fc::assert_exception&)
    {
    }

    // Get possible keys if memo was an account password
    std::string owner_seed = account.name + "owner" + memo;
    auto owner_secret = fc::sha256::hash(owner_seed.c_str(), owner_seed.size());
    keys.push_back(fc::ecc::private_key::regenerate(owner_secret).get_public_key());

    std::string active_seed = account.name + "active" + memo;
    auto active_secret = fc::sha256::hash(active_seed.c_str(), active_seed.size());
    keys.push_back(fc::ecc::private_key::regenerate(active_secret).get_public_key());

    std::string posting_seed = account.name + "posting" + memo;
    auto posting_secret = fc::sha256::hash(posting_seed.c_str(), posting_seed.size());
    keys.push_back(fc::ecc::private_key::regenerate(posting_secret).get_public_key());

    // Check keys against public keys in authorites
    for (auto& key_weight_pair : auth.owner.key_auths)
    {
        for (auto& key : keys)
            SCORUM_ASSERT(key_weight_pair.first != key, chain::plugin_exception,
                          "Detected private owner key in memo field. You should change your owner keys.");
    }

    for (auto& key_weight_pair : auth.active.key_auths)
    {
        for (auto& key : keys)
            SCORUM_ASSERT(key_weight_pair.first != key, chain::plugin_exception,
                          "Detected private active key in memo field. You should change your active keys.");
    }

    for (auto& key_weight_pair : auth.posting.key_auths)
    {
        for (auto& key : keys)
            SCORUM_ASSERT(key_weight_pair.first != key, chain::plugin_exception,
                          "Detected private posting key in memo field. You should change your posting keys.");
    }

    const auto& memo_key = account.memo_key;
    for (auto& key : keys)
        SCORUM_ASSERT(memo_key != key, chain::plugin_exception,
                      "Detected private memo key in memo field. You should change your memo key.");
}

struct operation_visitor
{
    operation_visitor(const chain::database& db)
        : _db(db)
    {
    }

    const chain::database& _db;

    typedef void result_type;

    template <typename T> void operator()(const T&) const
    {
    }

    void operator()(const comment_options_operation& o) const
    {
        for (auto& e : o.extensions)
        {
            const comment_payout_beneficiaries& cpb = e.get<comment_payout_beneficiaries>();
            SCORUM_ASSERT(cpb.beneficiaries.size() <= 8, chain::plugin_exception,
                          "Cannot specify more than 8 beneficiaries.");
        }
    }

    void operator()(const comment_operation& o) const
    {
        auto& comment_service = _db.obtain_service<dbs_comment>();

        if (o.parent_author != SCORUM_ROOT_POST_PARENT_ACCOUNT)
        {
            if (comment_service.is_exists(o.parent_author, o.parent_permlink))
            {
                const auto& parent = comment_service.get(o.parent_author, o.parent_permlink);

                SCORUM_ASSERT(parent.depth < SCORUM_SOFT_MAX_COMMENT_DEPTH, chain::plugin_exception,
                              "Comment is nested ${x} posts deep, maximum depth is ${y}.",
                              ("x", parent.depth)("y", SCORUM_SOFT_MAX_COMMENT_DEPTH));
            }
        }

        if (comment_service.is_exists(o.author, o.permlink))
        {
            const auto& comment = comment_service.get(o.author, o.permlink);

            if (comment.cashout_time == fc::time_point_sec::maximum())
            {
                FC_THROW_EXCEPTION(chain::plugin_exception, "The comment is archived");
            }
        }
    }

    void operator()(const transfer_operation& o) const
    {
        if (o.memo.length() > 0)
            check_memo(o.memo, _db.get<account_object, chain::by_name>(o.from),
                       _db.get<account_authority_object, chain::by_account>(o.from));
    }
};

void witness_plugin_impl::pre_transaction(const signed_transaction& trx)
{
    const auto& _db = _self.database();
    flat_set<account_name_type> required;
    std::vector<authority> other;
    trx.get_required_authorities(required, required, required, other);

    auto trx_size = fc::raw::pack_size(trx);

    for (const auto& auth : required)
    {
        const auto& acnt = _db.obtain_service<dbs_account>().get_account(auth);

        update_account_bandwidth(acnt, trx_size, bandwidth_type::forum);

        for (const auto& op : trx.operations)
        {
            if (is_market_operation(op))
            {
                update_account_bandwidth(acnt, trx_size * 10, bandwidth_type::market);
                break;
            }
        }
    }
}

void witness_plugin_impl::pre_operation(const operation_notification& note)
{
    const auto& _db = _self.database();
    if (_db.is_producing())
    {
        note.op.visit(operation_visitor(_db));
    }
}

void witness_plugin_impl::on_block(const signed_block& b)
{
    auto& db = _self.database();
    int64_t max_block_size
        = db.obtain_service<dbs_dynamic_global_property>().get().median_chain_props.maximum_block_size;

    auto reserve_ratio_ptr = db.find(reserve_ratio_id_type());

    if (BOOST_UNLIKELY(reserve_ratio_ptr == nullptr))
    {
        db.create<reserve_ratio_object>([&](reserve_ratio_object& r) {
            r.average_block_size = 0;
            r.current_reserve_ratio = SCORUM_MAX_RESERVE_RATIO * RESERVE_RATIO_PRECISION;
            r.max_virtual_bandwidth = (uint128_t(SCORUM_MAX_BLOCK_SIZE) * SCORUM_MAX_RESERVE_RATIO
                                       * SCORUM_BANDWIDTH_PRECISION * SCORUM_BANDWIDTH_AVERAGE_WINDOW_SECONDS)
                / SCORUM_BLOCK_INTERVAL;
        });
    }
    else
    {
        db.modify(*reserve_ratio_ptr, [&](reserve_ratio_object& r) {
            r.average_block_size = (99 * r.average_block_size + fc::raw::pack_size(b)) / 100;

            /**
            * About once per minute the average network use is consulted and used to
            * adjust the reserve ratio. Anything above 25% usage reduces the reserve
            * ratio proportional to the distance from 25%. If usage is at 50% then
            * the reserve ratio will half. Likewise, if it is at 12% it will increase by 50%.
            *
            * If the reserve ratio is consistently low, then it is probably time to increase
            * the capacity of the network.
            *
            * This algorithm is designed to react quickly to observations significantly
            * different from past observed behavior and make small adjustments when
            * behavior is within expected norms.
            */
            if (db.head_block_num() % 20 == 0)
            {
                int64_t distance
                    = ((r.average_block_size - (max_block_size / 4)) * DISTANCE_CALC_PRECISION) / (max_block_size / 4);
                auto old_reserve_ratio = r.current_reserve_ratio;

                if (distance > 0)
                {
                    r.current_reserve_ratio
                        -= (r.current_reserve_ratio * distance) / (distance + DISTANCE_CALC_PRECISION);

                    // We do not want the reserve ratio to drop below 1
                    if (r.current_reserve_ratio < RESERVE_RATIO_PRECISION)
                    {
                        r.current_reserve_ratio = RESERVE_RATIO_PRECISION;
                    }
                }
                else
                {
                    // By default, we should always slowly increase the reserve ratio.
                    r.current_reserve_ratio
                        += std::max(RESERVE_RATIO_MIN_INCREMENT,
                                    (r.current_reserve_ratio * distance) / (distance - DISTANCE_CALC_PRECISION));

                    if (r.current_reserve_ratio > SCORUM_MAX_RESERVE_RATIO * RESERVE_RATIO_PRECISION)
                    {
                        r.current_reserve_ratio = SCORUM_MAX_RESERVE_RATIO * RESERVE_RATIO_PRECISION;
                    }
                }

                if (old_reserve_ratio != r.current_reserve_ratio)
                {
                    ilog("Reserve ratio updated from ${old} to ${new}. Block: ${blocknum}",
                         ("old", old_reserve_ratio)("new", r.current_reserve_ratio)("blocknum", db.head_block_num()));
                }

                r.max_virtual_bandwidth
                    = (uint128_t(max_block_size) * uint128_t(r.current_reserve_ratio)
                       * uint128_t(SCORUM_BANDWIDTH_PRECISION * SCORUM_BANDWIDTH_AVERAGE_WINDOW_SECONDS))
                    / (SCORUM_BLOCK_INTERVAL * RESERVE_RATIO_PRECISION);
            }
        });
    }
}

void witness_plugin_impl::update_account_bandwidth(const account_object& a,
                                                   uint32_t trx_size,
                                                   const bandwidth_type type)
{
    database& _db = _self.database();
    const auto& props = _db.obtain_service<dbs_dynamic_global_property>().get();
    bool has_bandwidth = true;

    if (props.total_scorumpower.amount > 0)
    {
        auto band = _db.find<account_bandwidth_object, by_account_bandwidth_type>(boost::make_tuple(a.name, type));

        if (band == nullptr)
        {
            band = &_db.create<account_bandwidth_object>([&](account_bandwidth_object& b) {
                b.account = a.name;
                b.type = type;
            });
        }

        share_type new_bandwidth;
        share_type trx_bandwidth = trx_size * SCORUM_BANDWIDTH_PRECISION;
        auto delta_time = (_db.head_block_time() - band->last_bandwidth_update).to_seconds();

        if (delta_time > SCORUM_BANDWIDTH_AVERAGE_WINDOW_SECONDS)
        {
            new_bandwidth = 0;
        }
        else
            new_bandwidth
                = (((SCORUM_BANDWIDTH_AVERAGE_WINDOW_SECONDS - delta_time) * fc::uint128(band->average_bandwidth.value))
                   / SCORUM_BANDWIDTH_AVERAGE_WINDOW_SECONDS)
                      .to_uint64();

        new_bandwidth += trx_bandwidth;

        _db.modify(*band, [&](account_bandwidth_object& b) {
            b.average_bandwidth = new_bandwidth;
            b.lifetime_bandwidth += trx_bandwidth;
            b.last_bandwidth_update = _db.head_block_time();
        });

        fc::uint128 account_vshares(a.effective_scorumpower().amount.value);
        fc::uint128 total_vshares(props.total_scorumpower.amount.value);
        fc::uint128 account_average_bandwidth(band->average_bandwidth.value);
        fc::uint128 max_virtual_bandwidth(_db.get(reserve_ratio_id_type()).max_virtual_bandwidth);

        has_bandwidth = (account_vshares * max_virtual_bandwidth) > (account_average_bandwidth * total_vshares);

        if (_db.is_producing())
            SCORUM_ASSERT(has_bandwidth, chain::plugin_exception,
                          "Account: ${account} bandwidth limit exceeded. Please wait to transact or power up SCR.",
                          ("account", a.name)("account_vshares", account_vshares)("account_average_bandwidth",
                                                                                  account_average_bandwidth)(
                              "max_virtual_bandwidth", max_virtual_bandwidth)("total_scorumpower", total_vshares));
    }
}
}

witness_plugin::witness_plugin(application* app)
    : plugin(app)
    , _my(new detail::witness_plugin_impl(*this))
{
}

witness_plugin::~witness_plugin()
{
    try
    {
        if (_block_production_task.valid())
        {
            _block_production_task.cancel_and_wait(__FUNCTION__);
        }
    }
    catch (fc::canceled_exception&)
    {
        // Expected exception. Move along.
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
    }
}

void witness_plugin::plugin_set_program_options(boost::program_options::options_description& command_line_options,
                                                boost::program_options::options_description& config_file_options)
{
    std::string witness_id_example = "initwitness";
    command_line_options.add_options()("enable-stale-production",
                                       bpo::bool_switch()->notifier([this](bool e) { _production_enabled = e; }),
                                       "Enable block production, even if the chain is stale.")(
        "required-participation", bpo::bool_switch()->notifier([this](int e) {
            _required_witness_participation = uint32_t(e * SCORUM_1_PERCENT);
        }),
        "Percent of witnesses (0-99) that must be participating in order to produce blocks")(
        "witness,w", bpo::value<std::vector<std::string>>()->composing()->multitoken(),
        ("name of witness controlled by this node (e.g. " + witness_id_example + " )").c_str())(
        "private-key", bpo::value<std::vector<std::string>>()->composing()->multitoken(),
        "WIF PRIVATE KEY to be used by one or more witnesses or miners");
    config_file_options.add(command_line_options);
}

std::string witness_plugin::plugin_name() const
{
    return "witness";
}

void witness_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    try
    {
        // This plugin has API that push block to DB.
        // It is not expected for read-only mode
        ban_for_read_only_mode(options);

        _options = &options;
        LOAD_VALUE_SET(options, "witness", _witnesses, std::string)

        if (options.count("private-key"))
        {
            const std::vector<std::string> keys = options["private-key"].as<std::vector<std::string>>();
            for (const std::string& wif_key : keys)
            {
                fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(wif_key);
                FC_ASSERT(private_key.valid(), "unable to parse private key");
                _private_keys[private_key->get_public_key()] = *private_key;
            }
        }

        chain::database& db = database();

        db.on_pre_apply_transaction.connect([&](const signed_transaction& tx) { _my->pre_transaction(tx); });
        db.pre_apply_operation.connect([&](const operation_notification& note) { _my->pre_operation(note); });
        db.applied_block.connect([&](const signed_block& b) { _my->on_block(b); });

        db.add_plugin_index<account_bandwidth_index>();
        db.add_plugin_index<reserve_ratio_index>();
    }
    FC_LOG_AND_RETHROW()

    print_greeting();
}

void witness_plugin::plugin_startup()
{
    try
    {
        ilog("witness plugin:  plugin_startup() begin");

        if (!_witnesses.empty())
        {
            ilog("Launching block production for ${n} witnesses.", ("n", _witnesses.size()));
            idump((_witnesses));
            app().set_block_production(true);
            if (_production_enabled)
            {
                _production_skip_flags |= scorum::chain::database::skip_undo_history_check;
            }
            schedule_production_loop();
        }
        else
        {
            elog("No witnesses configured! Please add witness names and private keys to configuration.");
        }
        ilog("witness plugin:  plugin_startup() end");
    }
    FC_CAPTURE_AND_RETHROW()
}

void witness_plugin::plugin_shutdown()
{
    return;
}

void witness_plugin::schedule_production_loop()
{
    static const int64_t ONE_SECOND_MS = 1000000;
    // Schedule for the next second's tick regardless of chain state
    // If we would wait less than 50ms, wait for the whole second.
    fc::time_point fc_now = fc::time_point::now();
    int64_t time_to_next_second = ONE_SECOND_MS - (fc_now.time_since_epoch().count() % ONE_SECOND_MS);
    if (time_to_next_second < 50000) // we must sleep for at least 50ms
    {
        time_to_next_second += ONE_SECOND_MS;
    }

    fc::time_point next_wakeup(fc_now + fc::microseconds(time_to_next_second));

    // wdump( (now.time_since_epoch().count())(next_wakeup.time_since_epoch().count()) );
    _block_production_task = fc::schedule([this] { block_production_loop(); }, next_wakeup, "Witness Block Production");
}

void witness_plugin::block_production_loop()
{
    const fc::time_point genesis_time = database().get_genesis_time();

    if (fc::time_point::now() < genesis_time)
    {
        wlog("waiting until genesis time to produce block: ${t}", ("t", genesis_time));
        schedule_production_loop();
        return;
    }

    block_production_condition::block_production_condition_enum result;
    fc::mutable_variant_object capture;
    try
    {
        result = maybe_produce_block(capture);
    }
    catch (const fc::canceled_exception&)
    {
        // We're trying to exit. Go ahead and let this one out.
        throw;
    }
    catch (const scorum::chain::unknown_hardfork_exception& e)
    {
        // Hit a hardfork that the current node know nothing about, stop production and inform user
        elog("${e}\nNode may be out of date...", ("e", e.to_detail_string()));
        throw;
    }
    catch (const fc::exception& e)
    {
        elog("Got exception while generating block:\n${e}", ("e", e.to_detail_string()));
        result = block_production_condition::exception_producing_block;
    }

    switch (result)
    {
    case block_production_condition::produced:
        ilog("Generated block #${n} with timestamp ${t} at time ${c} by ${w}", (capture));
        break;
    case block_production_condition::not_synced:
        // ilog("Not producing block because production is disabled until we receive a recent block (see:
        // --enable-stale-production)");
        break;
    case block_production_condition::not_my_turn:
        // ilog("Not producing block because it isn't my turn");
        break;
    case block_production_condition::not_time_yet:
        // ilog("Not producing block because slot has not yet arrived");
        break;
    case block_production_condition::no_private_key:
        ilog("Not producing block for ${scheduled_witness} because I don't have the private key for ${scheduled_key}",
             (capture));
        break;
    case block_production_condition::low_participation:
        elog(
            "Not producing block because node appears to be on a minority fork with only ${pct}% witness participation",
            (capture));
        break;
    case block_production_condition::lag:
        elog("Not producing block because node didn't wake up within 500ms of the slot time.");
        break;
    case block_production_condition::consecutive:
        elog("Not producing block because the last block was generated by the same witness.\nThis node is probably "
             "disconnected from the network so block production has been disabled.\nDisable this check with "
             "--allow-consecutive option.");
        break;
    case block_production_condition::exception_producing_block:
        elog("Failure when producing block with no transactions");
        break;
    case block_production_condition::wait_for_genesis:
        break;
    }

    dlog("result = ${r}", ("r", result));
    schedule_production_loop();
}

block_production_condition::block_production_condition_enum
witness_plugin::maybe_produce_block(fc::mutable_variant_object& capture)
{
    chain::database& db = database();
    fc::time_point now_fine = fc::time_point::now();
    fc::time_point_sec now = now_fine + fc::microseconds(500000);

    // If the next block production opportunity is in the present or future, we're synced.
    if (!_production_enabled)
    {
        if (db.get_slot_time(1) >= now)
        {
            _production_enabled = true;
        }
        else
        {
            return block_production_condition::not_synced;
        }
    }

    // is anyone scheduled to produce now or one second in the future?
    uint32_t slot = db.get_slot_at_time(now);
    if (slot == 0)
    {
        capture("next_time", db.get_slot_time(1));
        return block_production_condition::not_time_yet;
    }

    //
    // this assert should not fail, because now <= db.head_block_time()
    // should have resulted in slot == 0.
    //
    // if this assert triggers, there is a serious bug in get_slot_at_time()
    // which would result in allowing a later block to have a timestamp
    // less than or equal to the previous block
    //
    assert(now > db.head_block_time());

    std::string scheduled_witness = db.get_scheduled_witness(slot);
    // we must control the witness scheduled to produce the next block.
    if (_witnesses.find(scheduled_witness) == _witnesses.end())
    {
        capture("scheduled_witness", scheduled_witness);
        return block_production_condition::not_my_turn;
    }

    const auto& witness_by_name = db.get_index<chain::witness_index>().indices().get<chain::by_name>();
    auto itr = witness_by_name.find(scheduled_witness);

    fc::time_point_sec scheduled_time = db.get_slot_time(slot);
    scorum::protocol::public_key_type scheduled_key = itr->signing_key;
    auto private_key_itr = _private_keys.find(scheduled_key);

    if (private_key_itr == _private_keys.end())
    {
        capture("scheduled_witness", scheduled_witness);
        capture("scheduled_key", scheduled_key);
        return block_production_condition::no_private_key;
    }

    uint32_t prate = db.witness_participation_rate();
    if (prate < _required_witness_participation)
    {
        capture("pct", uint32_t(100 * uint64_t(prate) / SCORUM_1_PERCENT));
        return block_production_condition::low_participation;
    }

    fc::microseconds dlt = scheduled_time - now;
    dlog("scheduled_time (${s} s) - now (${n} s) = ${d} ms",
         ("s", scheduled_time.sec_since_epoch())("n", now.sec_since_epoch())("d", dlt.count()));

    if (llabs(dlt.count()) > fc::milliseconds(500).count())
    {
        capture("scheduled_time", scheduled_time)("now", now);
        return block_production_condition::lag;
    }

    int retry = 0;
    do
    {
        try
        {
            auto block
                = db.generate_block(scheduled_time, scheduled_witness, private_key_itr->second, _production_skip_flags);
            capture("n", block.block_num())("t", block.timestamp)("c", now)("w", scheduled_witness);
            fc::async([this, block]() { p2p_node().broadcast(graphene::net::block_message(block)); });

            return block_production_condition::produced;
        }
        catch (fc::exception& e)
        {
            elog("${e}", ("e", e.to_detail_string()));
            elog("Clearing pending transactions and attempting again");
            db.clear_pending();
            retry++;
        }
    } while (retry < 2);

    return block_production_condition::exception_producing_block;
}
}
} // scorum::witness

SCORUM_DEFINE_PLUGIN(witness, scorum::witness::witness_plugin)
