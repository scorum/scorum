#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>
#include <openssl/md5.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/core/ignore_unused.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/container/deque.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/util/asset.hpp>

#include <scorum/chain/shared_db_merkle.hpp>
#include <scorum/chain/operation_notification.hpp>

#include <scorum/chain/database/database.hpp>
#include <scorum/chain/database_exceptions.hpp>
#include <scorum/chain/db_with.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <scorum/chain/schema/atomicswap_objects.hpp>
#include <scorum/chain/schema/block_summary_object.hpp>
#include <scorum/chain/schema/block_summary_object.hpp>
#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/chain_property_object.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/reward_balancer_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/transaction_object.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/atomicswap.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/witness_schedule.hpp>

#include <scorum/chain/database/block_tasks/process_comments_cashout.hpp>
#include <scorum/chain/database/block_tasks/process_funds.hpp>
#include <scorum/chain/database/block_tasks/process_vesting_withdrawals.hpp>

#include <scorum/chain/evaluators/evaluator_registry.hpp>
#include <scorum/chain/evaluators/proposal_create_evaluator.hpp>
#include <scorum/chain/evaluators/proposal_vote_evaluator.hpp>
#include <scorum/chain/evaluators/registration_pool_evaluator.hpp>
#include <scorum/chain/evaluators/scorum_evaluators.hpp>
#include <scorum/chain/evaluators/set_withdraw_scorumpower_route_evaluators.hpp>
#include <scorum/chain/evaluators/withdraw_scorumpower_evaluator.hpp>

namespace scorum {
namespace chain {

class database_impl
{
public:
    database_impl(database& self);

    database& _self;
    evaluator_registry<operation> _evaluator_registry;
    genesis_persistent_state_type _genesis_persistent_state;

    database_ns::process_funds _process_funds;
    database_ns::process_comments_cashout _process_comments_cashout;
    database_ns::process_vesting_withdrawals _process_vesting_withdrawals;
};

database_impl::database_impl(database& self)
    : _self(self)
    , _evaluator_registry(self)
{
}

database::database(uint32_t options)
    : chainbase::database()
    , dbservice_dbs_factory(*this)
    , data_service_factory(*this)
    , _my(new database_impl(*this))
    , _options(options)
{
}

database::~database()
{
    clear_pending();
}

void database::open(const fc::path& data_dir,
                    const fc::path& shared_mem_dir,
                    uint64_t shared_file_size,
                    uint32_t chainbase_flags,
                    const genesis_state_type& genesis_state)
{
    try
    {
        chainbase::database::open(shared_mem_dir, chainbase_flags, shared_file_size);

        // must be initialized before evaluators creation
        _my->_genesis_persistent_state = static_cast<const genesis_persistent_state_type&>(genesis_state);

        initialize_indexes();
        initialize_evaluators();

        if (chainbase_flags & chainbase::database::read_write)
        {
            if (!find<dynamic_global_property_object>())
                with_write_lock([&]() { init_genesis(genesis_state); });

            if (!fc::exists(data_dir))
            {
                fc::create_directories(data_dir);
            }

            _block_log.open(data_dir / "block_log");

            auto log_head = _block_log.head();

            // Rewind all undo state. This should return us to the state at the last irreversible block.
            with_write_lock([&]() {
                for_each_index([&](chainbase::abstract_generic_index_i& item) { item.undo_all(); });

                for_each_index([&](chainbase::abstract_generic_index_i& item) {
                    FC_ASSERT(item.revision() == head_block_num(), "Chainbase revision does not match head block num",
                              ("rev", item.revision())("head_block", head_block_num()));
                });

                validate_invariants();
            });

            if (head_block_num())
            {
                auto head_block = _block_log.read_block_by_num(head_block_num());
                // This assertion should be caught and a reindex should occur
                FC_ASSERT(head_block.valid() && head_block->id() == head_block_id(),
                          "Chain state does not match block log. Please reindex blockchain.");

                _fork_db.start_block(*head_block);
            }
        }

        try
        {
            const auto& chain_id = get<chain_property_object>().chain_id;
            FC_ASSERT(genesis_state.initial_chain_id == chain_id,
                      "Current chain id is not equal initial chain id = ${id}", ("id", chain_id));
        }
        catch (fc::exception& er)
        {
            throw std::logic_error(std::string("Invalid chain id: ") + er.to_detail_string());
        }

        with_read_lock([&]() {
            init_hardforks(genesis_state.initial_timestamp); // Writes to local state, but reads from db
        });
    }
    FC_CAPTURE_LOG_AND_RETHROW((data_dir)(shared_mem_dir)(shared_file_size))
}

void database::reindex(const fc::path& data_dir,
                       const fc::path& shared_mem_dir,
                       uint64_t shared_file_size,
                       const genesis_state_type& genesis_state)
{
    try
    {
        ilog("Reindexing Blockchain");
        wipe(data_dir, shared_mem_dir, false);
        open(data_dir, shared_mem_dir, shared_file_size, chainbase::database::read_write, genesis_state);
        _fork_db.reset(); // override effect of _fork_db.start_block() call in open()

        auto start = fc::time_point::now();
        SCORUM_ASSERT(_block_log.head(), block_log_exception, "No blocks in block log. Cannot reindex an empty chain.");

        ilog("Replaying blocks...");

        uint64_t skip_flags = skip_witness_signature | skip_transaction_signatures | skip_transaction_dupe_check
            | skip_tapos_check | skip_merkle_check | skip_witness_schedule_check | skip_authority_check | skip_validate
            | /// no need to validate operations
            skip_validate_invariants | skip_block_log;

        with_write_lock([&]() {
            auto itr = _block_log.read_block(0);
            auto last_block_num = _block_log.head()->block_num();

            while (itr.first.block_num() != last_block_num)
            {
                auto cur_block_num = itr.first.block_num();
                if (cur_block_num % 100000 == 0)
                    std::cerr << "   " << double(cur_block_num * 100) / last_block_num << "%   " << cur_block_num
                              << " of " << last_block_num << "   (" << (get_free_memory() / (1024 * 1024))
                              << "M free)\n";
                apply_block(itr.first, skip_flags);
                itr = _block_log.read_block(itr.second);
            }

            apply_block(itr.first, skip_flags);

            for_each_index([&](chainbase::abstract_generic_index_i& item) { item.set_revision(head_block_num()); });
        });

        if (_block_log.head()->block_num())
        {
            _fork_db.start_block(*_block_log.head());
        }

        auto end = fc::time_point::now();
        ilog("Done reindexing, elapsed time: ${t} sec", ("t", double((end - start).count()) / 1000000.0));
    }
    FC_CAPTURE_AND_RETHROW((data_dir)(shared_mem_dir))
}

void database::wipe(const fc::path& data_dir, const fc::path& shared_mem_dir, bool include_blocks)
{
    close();
    chainbase::database::wipe();
    if (include_blocks)
    {
        fc::remove_all(data_dir / "block_log");
        fc::remove_all(data_dir / "block_log.index");
    }
}

void database::close()
{
    try
    {
        // Since pop_block() will move tx's in the popped blocks into pending,
        // we have to clear_pending() after we're done popping to get a clean
        // DB state (issue #336).
        clear_pending();

        try
        {
            chainbase::database::flush();
        }
        catch (...)
        {
        }

        chainbase::database::close();

        _block_log.close();

        _fork_db.reset();
    }
    FC_CAPTURE_AND_RETHROW()
}

bool database::is_known_block(const block_id_type& id) const
{
    try
    {
        return fetch_block_by_id(id).valid();
    }
    FC_CAPTURE_AND_RETHROW()
}

/**
 * Only return true *if* the transaction has not expired or been invalidated. If this
 * method is called with a VERY old transaction we will return false, they should
 * query things by blocks if they are that old.
 */
bool database::is_known_transaction(const transaction_id_type& id) const
{
    try
    {
        const auto& trx_idx = get_index<transaction_index>().indices().get<by_trx_id>();
        return trx_idx.find(id) != trx_idx.end();
    }
    FC_CAPTURE_AND_RETHROW()
}

block_id_type database::find_block_id_for_num(uint32_t block_num) const
{
    try
    {
        if (block_num == 0)
        {
            return block_id_type();
        }

        // Reversible blocks are *usually* in the TAPOS buffer.  Since this
        // is the fastest check, we do it first.
        block_summary_id_type bsid = block_num & (uint32_t)SCORUM_BLOCKID_POOL_SIZE;
        const block_summary_object* bs = find<block_summary_object, by_id>(bsid);
        if (bs != nullptr)
        {
            if (protocol::block_header::num_from_id(bs->block_id) == block_num)
            {
                return bs->block_id;
            }
        }

        // Next we query the block log.   Irreversible blocks are here.
        auto b = _block_log.read_block_by_num(block_num);
        if (b.valid())
        {
            return b->id();
        }

        // Finally we query the fork DB.
        std::shared_ptr<fork_item> fitem = _fork_db.fetch_block_on_main_branch_by_number(block_num);
        if (fitem)
        {
            return fitem->id;
        }

        return block_id_type();
    }
    FC_CAPTURE_AND_RETHROW((block_num))
}

block_id_type database::get_block_id_for_num(uint32_t block_num) const
{
    block_id_type bid = find_block_id_for_num(block_num);
    FC_ASSERT(bid != block_id_type());
    return bid;
}

optional<signed_block> database::fetch_block_by_id(const block_id_type& id) const
{
    try
    {
        auto b = _fork_db.fetch_block(id);
        if (!b)
        {
            auto tmp = _block_log.read_block_by_num(protocol::block_header::num_from_id(id));

            if (tmp && tmp->id() == id)
            {
                return tmp;
            }

            tmp.reset();
            return tmp;
        }

        return b->data;
    }
    FC_CAPTURE_AND_RETHROW()
}

optional<signed_block> database::fetch_block_by_number(uint32_t block_num) const
{
    try
    {
        optional<signed_block> b;

        auto results = _fork_db.fetch_block_by_number(block_num);
        if (results.size() == 1)
        {
            b = results[0]->data;
        }
        else
        {
            b = _block_log.read_block_by_num(block_num);
        }

        return b;
    }
    FC_LOG_AND_RETHROW()
}

optional<signed_block> database::read_block_by_number(uint32_t block_num) const
{
    return _block_log.read_block_by_num(block_num);
}

const signed_transaction database::get_recent_transaction(const transaction_id_type& trx_id) const
{
    try
    {
        auto& index = get_index<transaction_index>().indices().get<by_trx_id>();
        auto itr = index.find(trx_id);
        FC_ASSERT(itr != index.end());
        signed_transaction trx;
        fc::raw::unpack(itr->packed_trx, trx);
        return trx;
        ;
    }
    FC_CAPTURE_AND_RETHROW()
}

std::vector<block_id_type> database::get_block_ids_on_fork(block_id_type head_of_fork) const
{
    try
    {
        std::pair<fork_database::branch_type, fork_database::branch_type> branches
            = _fork_db.fetch_branch_from(head_block_id(), head_of_fork);
        if (!((branches.first.back()->previous_id() == branches.second.back()->previous_id())))
        {
            edump((head_of_fork)(head_block_id())(branches.first.size())(branches.second.size()));
            assert(branches.first.back()->previous_id() == branches.second.back()->previous_id());
        }
        std::vector<block_id_type> result;
        for (const item_ptr& fork_block : branches.second)
        {
            result.emplace_back(fork_block->id);
        }
        result.emplace_back(branches.first.back()->previous_id());
        return result;
    }
    FC_CAPTURE_AND_RETHROW()
}

chain_id_type database::get_chain_id() const
{
    return get<chain_property_object>().chain_id;
}

const node_property_object& database::get_node_properties() const
{
    return _node_property_object;
}

const time_point_sec database::calculate_discussion_payout_time(const comment_object& comment) const
{
    return comment.cashout_time;
}

uint32_t database::witness_participation_rate() const
{
    const dynamic_global_property_object& dpo = obtain_service<dbs_dynamic_global_property>().get();
    return uint64_t(SCORUM_100_PERCENT) * dpo.recent_slots_filled.popcount() / 128;
}

void database::add_checkpoints(const flat_map<uint32_t, block_id_type>& checkpts)
{
    for (const auto& i : checkpts)
    {
        _checkpoints[i.first] = i.second;
    }
}

bool database::before_last_checkpoint() const
{
    return (_checkpoints.size() > 0) && (_checkpoints.rbegin()->first >= head_block_num());
}

/**
 * Push block "may fail" in which case every partial change is unwound.  After
 * push block is successful the block is appended to the chain database on disk.
 *
 * @return true if we switched forks as a result of this push.
 */
bool database::push_block(const signed_block& new_block, uint32_t skip)
{
    // fc::time_point begin_time = fc::time_point::now();

    bool result;
    detail::with_skip_flags(*this, skip, [&]() {
        with_write_lock([&]() {
            detail::without_pending_transactions(*this, std::move(_pending_tx), [&]() {
                try
                {
                    result = _push_block(new_block);
                }
                FC_CAPTURE_AND_RETHROW((new_block))
            });
        });
    });

    // fc::time_point end_time = fc::time_point::now();
    // fc::microseconds dt = end_time - begin_time;
    // if( ( new_block.block_num() % 10000 ) == 0 )
    //   ilog( "push_block ${b} took ${t} microseconds", ("b", new_block.block_num())("t", dt.count()) );
    return result;
}

void database::_maybe_warn_multiple_production(uint32_t height) const
{
    auto blocks = _fork_db.fetch_block_by_number(height);
    if (blocks.size() > 1)
    {
        std::vector<std::pair<account_name_type, fc::time_point_sec>> witness_time_pairs;
        for (const auto& b : blocks)
        {
            witness_time_pairs.push_back(std::make_pair(b->data.witness, b->data.timestamp));
        }

        ilog("Encountered block num collision at block ${n} due to a fork, witnesses are: ${w}",
             ("n", height)("w", witness_time_pairs));
    }
    return;
}

bool database::_push_block(const signed_block& new_block)
{
    try
    {
        uint32_t skip = get_node_properties().skip_flags;
        // uint32_t skip_undo_db = skip & skip_undo_block;

        if (!(skip & skip_fork_db))
        {
            std::shared_ptr<fork_item> new_head = _fork_db.push_block(new_block);
            _maybe_warn_multiple_production(new_head->num);

            // If the head block from the longest chain does not build off of the current head, we need to switch forks.
            if (new_head->data.previous != head_block_id())
            {
                // If the newly pushed block is the same height as head, we get head back in new_head
                // Only switch forks if new_head is actually higher than head
                if (new_head->data.block_num() > head_block_num())
                {
                    // wlog( "Switching to fork: ${id}", ("id",new_head->data.id()) );
                    auto branches = _fork_db.fetch_branch_from(new_head->data.id(), head_block_id());

                    // pop blocks until we hit the forked block
                    while (head_block_id() != branches.second.back()->data.previous)
                    {
                        pop_block();
                    }

                    // push all blocks on the new fork
                    for (auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr)
                    {
                        // ilog( "pushing blocks from fork ${n} ${id}",
                        // ("n",(*ritr)->data.block_num())("id",(*ritr)->data.id()) );
                        optional<fc::exception> except;
                        try
                        {
                            auto session = start_undo_session();
                            apply_block((*ritr)->data, skip);
                            session->push();
                        }
                        catch (const fc::exception& e)
                        {
                            except = e;
                        }
                        if (except)
                        {
                            // wlog( "exception thrown while switching forks ${e}", ("e",except->to_detail_string() ) );
                            // remove the rest of branches.first from the fork_db, those blocks are invalid
                            while (ritr != branches.first.rend())
                            {
                                _fork_db.remove((*ritr)->data.id());
                                ++ritr;
                            }
                            _fork_db.set_head(branches.second.front());

                            // pop all blocks from the bad fork
                            while (head_block_id() != branches.second.back()->data.previous)
                            {
                                pop_block();
                            }

                            // restore all blocks from the good fork
                            for (auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr)
                            {
                                auto session = start_undo_session();
                                apply_block((*ritr)->data, skip);
                                session->push();
                            }
                            throw * except;
                        }
                    }
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        try
        {
            auto session = start_undo_session();
            apply_block(new_block, skip);
            session->push();
        }
        catch (const fc::exception& e)
        {
            elog("Failed to push new block:\n${e}", ("e", e.to_detail_string()));
            _fork_db.remove(new_block.id());
            throw;
        }

        return false;
    }
    FC_CAPTURE_AND_RETHROW()
}

/**
 * Attempts to push the transaction into the pending queue
 *
 * When called to push a locally generated transaction, set the skip_block_size_check bit on the skip argument. This
 * will allow the transaction to be pushed even if it causes the pending block size to exceed the maximum block size.
 * Although the transaction will probably not propagate further now, as the peers are likely to have their pending
 * queues full as well, it will be kept in the queue to be propagated later when a new block flushes out the pending
 * queues.
 */
void database::push_transaction(const signed_transaction& trx, uint32_t skip)
{
    try
    {
        try
        {
            size_t trx_size = fc::raw::pack_size(trx);
            FC_ASSERT(
                trx_size
                <= (obtain_service<dbs_dynamic_global_property>().get().median_chain_props.maximum_block_size - 256));
            set_producing(true);
            detail::with_skip_flags(*this, skip, [&]() { with_write_lock([&]() { _push_transaction(trx); }); });
            set_producing(false);
        }
        catch (...)
        {
            set_producing(false);
            throw;
        }
    }
    FC_CAPTURE_AND_RETHROW((trx))
}

void database::_push_transaction(const signed_transaction& trx)
{
    // If this is the first transaction pushed after applying a block, start a new undo session.
    // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
    if (!_pending_tx_session.valid())
    {
        _pending_tx_session = start_undo_session();
    }

    // Create a temporary undo session as a child of _pending_tx_session.
    // The temporary session will be discarded by the destructor if
    // _apply_transaction fails.  If we make it to merge(), we
    // apply the changes.

    auto temp_session = start_undo_session();
    _apply_transaction(trx);
    _pending_tx.push_back(trx);

    // The transaction applied successfully. Merge its changes into the pending block session.
    for_each_index([&](chainbase::abstract_generic_index_i& item) { item.squash(); });
    temp_session->push();

    // notify anyone listening to pending transactions
    notify_on_pending_transaction(trx);
}

signed_block database::generate_block(fc::time_point_sec when,
                                      const account_name_type& witness_owner,
                                      const fc::ecc::private_key& block_signing_private_key,
                                      uint32_t skip /* = 0 */
                                      )
{
    signed_block result;
    detail::with_skip_flags(*this, skip, [&]() {
        try
        {
            result = _generate_block(when, witness_owner, block_signing_private_key);
        }
        FC_CAPTURE_AND_RETHROW((witness_owner))
    });
    return result;
}

signed_block database::_generate_block(fc::time_point_sec when,
                                       const account_name_type& witness_owner,
                                       const fc::ecc::private_key& block_signing_private_key)
{
    auto& witness_service = obtain_service<dbs_witness>();

    uint32_t skip = get_node_properties().skip_flags;
    uint32_t slot_num = get_slot_at_time(when);
    FC_ASSERT(slot_num > 0);
    std::string scheduled_witness = get_scheduled_witness(slot_num);
    FC_ASSERT(scheduled_witness == witness_owner);

    const auto& witness_obj = witness_service.get(witness_owner);

    if (!(skip & skip_witness_signature))
    {
        FC_ASSERT(witness_obj.signing_key == block_signing_private_key.get_public_key());
    }

    static const size_t max_block_header_size = fc::raw::pack_size(signed_block_header()) + 4;
    auto maximum_block_size = obtain_service<dbs_dynamic_global_property>()
                                  .get()
                                  .median_chain_props.maximum_block_size; // SCORUM_MAX_BLOCK_SIZE;
    size_t total_block_size = max_block_header_size;

    signed_block pending_block;

    with_write_lock([&]() {
        //
        // The following code throws away existing pending_tx_session and
        // rebuilds it by re-applying pending transactions.
        //
        // This rebuild is necessary because pending transactions' validity
        // and semantics may have changed since they were received, because
        // time-based semantics are evaluated based on the current block
        // time.  These changes can only be reflected in the database when
        // the value of the "when" variable is known, which means we need to
        // re-apply pending transactions in this method.
        //
        _pending_tx_session.reset();
        _pending_tx_session = start_undo_session();

        uint64_t postponed_tx_count = 0;
        // pop pending state (reset to head block state)
        for (const signed_transaction& tx : _pending_tx)
        {
            // Only include transactions that have not expired yet for currently generating block,
            // this should clear problem transactions and allow block production to continue

            if (tx.expiration < when)
            {
                continue;
            }

            uint64_t new_total_size = total_block_size + fc::raw::pack_size(tx);

            // postpone transaction if it would make block too big
            if (new_total_size >= maximum_block_size)
            {
                postponed_tx_count++;
                continue;
            }

            try
            {
                auto temp_session = start_undo_session();
                _apply_transaction(tx);
                for_each_index([&](chainbase::abstract_generic_index_i& item) { item.squash(); });
                temp_session->push();

                total_block_size += fc::raw::pack_size(tx);
                pending_block.transactions.push_back(tx);
            }
            catch (const fc::exception& e)
            {
                // Do nothing, transaction will not be re-applied
                // wlog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
                // wlog( "The transaction was ${t}", ("t", tx) );
            }
        }
        if (postponed_tx_count > 0)
        {
            wlog("Postponed ${n} transactions due to block size limit", ("n", postponed_tx_count));
        }

        _pending_tx_session.reset();
    });

    // We have temporarily broken the invariant that
    // _pending_tx_session is the result of applying _pending_tx, as
    // _pending_tx now consists of the set of postponed transactions.
    // However, the push_block() call below will re-create the
    // _pending_tx_session.

    pending_block.previous = head_block_id();
    pending_block.timestamp = when;
    pending_block.transaction_merkle_root = pending_block.calculate_merkle_root();
    pending_block.witness = witness_owner;

    const auto& witness = witness_service.get(witness_owner);

    if (witness.running_version != SCORUM_BLOCKCHAIN_VERSION)
    {
        pending_block.extensions.insert(block_header_extensions(SCORUM_BLOCKCHAIN_VERSION));
    }

    const auto& hfp = obtain_service<dbs_hardfork_property>().get();

    if (hfp.current_hardfork_version
            < SCORUM_BLOCKCHAIN_HARDFORK_VERSION // Binary is newer hardfork than has been applied
        && (witness.hardfork_version_vote != _hardfork_versions[hfp.last_hardfork + 1]
            || witness.hardfork_time_vote
                != _hardfork_times[hfp.last_hardfork + 1])) // Witness vote does not match binary configuration
    {
        // Make vote match binary configuration
        pending_block.extensions.insert(block_header_extensions(
            hardfork_version_vote(_hardfork_versions[hfp.last_hardfork + 1], _hardfork_times[hfp.last_hardfork + 1])));
    }
    else if (hfp.current_hardfork_version
                 == SCORUM_BLOCKCHAIN_HARDFORK_VERSION // Binary does not know of a new hardfork
             && witness.hardfork_version_vote
                 > SCORUM_BLOCKCHAIN_HARDFORK_VERSION) // Voting for hardfork in the future, that we do not know of...
    {
        // Make vote match binary configuration. This is vote to not apply the new hardfork.
        pending_block.extensions.insert(block_header_extensions(
            hardfork_version_vote(_hardfork_versions[hfp.last_hardfork], _hardfork_times[hfp.last_hardfork])));
    }

    if (!(skip & skip_witness_signature))
    {
        pending_block.sign(block_signing_private_key);
    }

    // TODO:  Move this to _push_block() so session is restored.
    if (!(skip & skip_block_size_check))
    {
        FC_ASSERT(fc::raw::pack_size(pending_block) <= SCORUM_MAX_BLOCK_SIZE);
    }

    push_block(pending_block, skip);

    return pending_block;
}

/**
 * Removes the most recent block from the database and
 * undoes any changes it made.
 */
void database::pop_block()
{
    try
    {
        _pending_tx_session.reset();
        auto head_id = head_block_id();

        /// save the head block so we can recover its transactions
        optional<signed_block> head_block = fetch_block_by_id(head_id);
        SCORUM_ASSERT(head_block.valid(), pop_empty_chain, "there are no blocks to pop");

        _fork_db.pop_block();

        for_each_index([&](chainbase::abstract_generic_index_i& item) { item.undo(); });

        _popped_tx.insert(_popped_tx.begin(), head_block->transactions.begin(), head_block->transactions.end());
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::clear_pending()
{
    try
    {
        assert((_pending_tx.size() == 0) || _pending_tx_session.valid());
        _pending_tx.clear();
        _pending_tx_session.reset();
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::notify_pre_apply_operation(operation_notification& note)
{
    note.trx_id = _current_trx_id;
    note.block = _current_block_num;
    note.trx_in_block = _current_trx_in_block;
    note.op_in_trx = _current_op_in_trx;

    SCORUM_TRY_NOTIFY(pre_apply_operation, note)
}

void database::notify_post_apply_operation(const operation_notification& note)
{
    SCORUM_TRY_NOTIFY(post_apply_operation, note)
}

inline void database::push_virtual_operation(const operation& op)
{
    if (_options & opt_notify_virtual_op_applying)
    {
        FC_ASSERT(is_virtual_operation(op));
        operation_notification note(op);
        notify_pre_apply_operation(note);
        notify_post_apply_operation(note);
    }
}

inline void database::push_hf_operation(const operation& op)
{
    FC_ASSERT(is_virtual_operation(op));
    operation_notification note(op);
    notify_pre_apply_operation(note);
    notify_post_apply_operation(note);
}

void database::notify_pre_applied_block(const signed_block& block)
{
    SCORUM_TRY_NOTIFY(pre_applied_block, block)
}

void database::notify_applied_block(const signed_block& block)
{
    SCORUM_TRY_NOTIFY(applied_block, block)
}

void database::notify_on_pending_transaction(const signed_transaction& tx)
{
    SCORUM_TRY_NOTIFY(on_pending_transaction, tx)
}

void database::notify_on_pre_apply_transaction(const signed_transaction& tx)
{
    SCORUM_TRY_NOTIFY(on_pre_apply_transaction, tx)
}

void database::notify_on_applied_transaction(const signed_transaction& tx)
{
    SCORUM_TRY_NOTIFY(on_applied_transaction, tx);
}

account_name_type database::get_scheduled_witness(uint32_t slot_num) const
{
    const dynamic_global_property_object& dpo = obtain_service<dbs_dynamic_global_property>().get();
    const witness_schedule_object& wso = obtain_service<dbs_witness_schedule>().get();

    uint64_t current_aslot = dpo.current_aslot + slot_num;
    return wso.current_shuffled_witnesses[current_aslot % wso.num_scheduled_witnesses];
}

fc::time_point_sec database::get_slot_time(uint32_t slot_num) const
{
    if (slot_num == 0)
    {
        return fc::time_point_sec();
    }

    auto interval = SCORUM_BLOCK_INTERVAL;
    const dynamic_global_property_object& dpo = obtain_service<dbs_dynamic_global_property>().get();

    if (head_block_num() == 0)
    {
        // n.b. first block is at genesis_time plus one block interval
        fc::time_point_sec genesis_time = dpo.time;
        return genesis_time + slot_num * interval;
    }

    int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
    fc::time_point_sec head_slot_time(head_block_abs_slot * interval);

    // "slot 0" is head_slot_time
    // "slot 1" is head_slot_time,
    //   plus maint interval if head block is a maint block
    //   plus block interval if head block is not a maint block
    return head_slot_time + (slot_num * interval);
}

uint32_t database::get_slot_at_time(fc::time_point_sec when) const
{
    fc::time_point_sec first_slot_time = get_slot_time(1);
    if (when < first_slot_time)
    {
        return 0;
    }
    return (when - first_slot_time).to_seconds() / SCORUM_BLOCK_INTERVAL + 1;
}

void database::account_recovery_processing()
{
    // Clear expired recovery requests
    const auto& rec_req_idx = get_index<account_recovery_request_index>().indices().get<by_expiration>();
    auto rec_req = rec_req_idx.begin();

    while (rec_req != rec_req_idx.end() && rec_req->expires <= head_block_time())
    {
        remove(*rec_req);
        rec_req = rec_req_idx.begin();
    }

    // Clear invalid historical authorities
    const auto& hist_idx = get_index<owner_authority_history_index>().indices(); // by id
    auto hist = hist_idx.begin();

    while (hist != hist_idx.end()
           && time_point_sec(hist->last_valid_time + SCORUM_OWNER_AUTH_RECOVERY_PERIOD) < head_block_time())
    {
        remove(*hist);
        hist = hist_idx.begin();
    }

    // Apply effective recovery_account changes
    const auto& change_req_idx = get_index<change_recovery_account_request_index>().indices().get<by_effective_date>();
    auto change_req = change_req_idx.begin();

    const dbs_account& account_service = obtain_service<dbs_account>();
    while (change_req != change_req_idx.end() && change_req->effective_on <= head_block_time())
    {
        modify(account_service.get_account(change_req->account_to_recover),
               [&](account_object& a) { a.recovery_account = change_req->recovery_account; });

        remove(*change_req);
        change_req = change_req_idx.begin();
    }
}

void database::expire_escrow_ratification()
{
    const auto& escrow_idx = get_index<escrow_index>().indices().get<by_ratification_deadline>();
    auto escrow_itr = escrow_idx.lower_bound(false);

    while (escrow_itr != escrow_idx.end() && !escrow_itr->is_approved()
           && escrow_itr->ratification_deadline <= head_block_time())
    {
        const auto& old_escrow = *escrow_itr;
        ++escrow_itr;

        const auto& from_account = obtain_service<dbs_account>().get_account(old_escrow.from);
        adjust_balance(from_account, old_escrow.scorum_balance);
        adjust_balance(from_account, old_escrow.pending_fee);

        remove(old_escrow);
    }
}

void database::process_decline_voting_rights()
{
    const auto& request_idx = get_index<decline_voting_rights_request_index>().indices().get<by_effective_date>();
    auto itr = request_idx.begin();

    dbs_account& account_service = obtain_service<dbs_account>();

    while (itr != request_idx.end() && itr->effective_date <= head_block_time())
    {
        const auto& account = get(itr->account);

        /// remove all current votes
        std::array<share_type, SCORUM_MAX_PROXY_RECURSION_DEPTH + 1> delta;
        delta[0] = -account.scorumpower.amount;
        for (int i = 0; i < SCORUM_MAX_PROXY_RECURSION_DEPTH; ++i)
        {
            delta[i + 1] = -account.proxied_vsf_votes[i];
        }
        account_service.adjust_proxied_witness_votes(account, delta);

        account_service.clear_witness_votes(account);

        modify(get(itr->account), [&](account_object& a) {
            a.can_vote = false;
            a.proxy = SCORUM_PROXY_TO_SELF_ACCOUNT;
        });

        remove(*itr);
        itr = request_idx.begin();
    }
}

time_point_sec database::head_block_time() const
{
    return obtain_service<dbs_dynamic_global_property>().get().time;
}

uint32_t database::head_block_num() const
{
    return obtain_service<dbs_dynamic_global_property>().get().head_block_number;
}

block_id_type database::head_block_id() const
{
    return obtain_service<dbs_dynamic_global_property>().get().head_block_id;
}

node_property_object& database::node_properties()
{
    return _node_property_object;
}

uint32_t database::last_non_undoable_block_num() const
{
    return obtain_service<dbs_dynamic_global_property>().get().last_irreversible_block_num;
}

void database::initialize_evaluators()
{
    _my->_evaluator_registry.register_evaluator<account_create_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_create_with_delegation_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_update_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_witness_proxy_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_witness_vote_evaluator>();
    _my->_evaluator_registry.register_evaluator<atomicswap_initiate_evaluator>();
    _my->_evaluator_registry.register_evaluator<atomicswap_redeem_evaluator>();
    _my->_evaluator_registry.register_evaluator<atomicswap_refund_evaluator>();
    _my->_evaluator_registry.register_evaluator<change_recovery_account_evaluator>();
    _my->_evaluator_registry.register_evaluator<close_budget_evaluator>();
    _my->_evaluator_registry.register_evaluator<comment_evaluator>();
    _my->_evaluator_registry.register_evaluator<comment_options_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_budget_evaluator>();
    _my->_evaluator_registry.register_evaluator<decline_voting_rights_evaluator>();
    _my->_evaluator_registry.register_evaluator<delegate_scorumpower_evaluator>();
    _my->_evaluator_registry.register_evaluator<delete_comment_evaluator>();
    _my->_evaluator_registry.register_evaluator<escrow_approve_evaluator>();
    _my->_evaluator_registry.register_evaluator<escrow_dispute_evaluator>();
    _my->_evaluator_registry.register_evaluator<escrow_release_evaluator>();
    _my->_evaluator_registry.register_evaluator<escrow_transfer_evaluator>();
    _my->_evaluator_registry.register_evaluator<prove_authority_evaluator>();
    _my->_evaluator_registry.register_evaluator<recover_account_evaluator>();
    _my->_evaluator_registry.register_evaluator<request_account_recovery_evaluator>();
    _my->_evaluator_registry.register_evaluator<transfer_evaluator>();
    _my->_evaluator_registry.register_evaluator<transfer_to_scorumpower_evaluator>();
    _my->_evaluator_registry.register_evaluator<vote_evaluator>();
    _my->_evaluator_registry.register_evaluator<witness_update_evaluator>();
    _my->_evaluator_registry.register_evaluator<proposal_create_evaluator>();
    _my->_evaluator_registry.register_evaluator<proposal_vote_evaluator>();
    _my->_evaluator_registry.register_evaluator<proposal_create_evaluator>();
    _my->_evaluator_registry.register_evaluator<set_withdraw_scorumpower_route_to_dev_pool_evaluator>();
    _my->_evaluator_registry.register_evaluator<set_withdraw_scorumpower_route_to_account_evaluator>();
    _my->_evaluator_registry.register_evaluator<withdraw_scorumpower_evaluator>();
    _my->_evaluator_registry.register_evaluator<registration_pool_evaluator>();
}

void database::initialize_indexes()
{
    add_index<account_authority_index>();
    add_index<account_index>();
    add_index<account_recovery_request_index>();
    add_index<block_summary_index>();
    add_index<budget_index>();
    add_index<chain_property_index>();
    add_index<change_recovery_account_request_index>();
    add_index<comment_index>();
    add_index<comment_vote_index>();
    add_index<decline_voting_rights_request_index>();
    add_index<dynamic_global_property_index>();
    add_index<escrow_index>();
    add_index<hardfork_property_index>();
    add_index<owner_authority_history_index>();
    add_index<proposal_object_index>();
    add_index<registration_committee_member_index>();
    add_index<registration_pool_index>();
    add_index<reward_fund_index>();
    add_index<reward_pool_index>();
    add_index<transaction_index>();
    add_index<scorumpower_delegation_expiration_index>();
    add_index<scorumpower_delegation_index>();
    add_index<withdraw_scorumpower_route_index>();
    add_index<withdraw_scorumpower_route_statistic_index>();
    add_index<withdraw_scorumpower_index>();
    add_index<witness_index>();
    add_index<witness_schedule_index>();
    add_index<witness_vote_index>();
    add_index<atomicswap_contract_index>();

    add_index<dev_committee_index>();
    add_index<dev_committee_member_index>();

    _plugin_index_signal();
}

void database::validate_transaction(const signed_transaction& trx)
{
    database::with_write_lock([&]() {
        auto session = start_undo_session();
        _apply_transaction(trx);
    });
}

void database::set_flush_interval(uint32_t flush_blocks)
{
    _flush_blocks = flush_blocks;
    _next_flush_block = 0;
}

//////////////////// private methods ////////////////////

void database::apply_block(const signed_block& next_block, uint32_t skip)
{
    try
    {
        // fc::time_point begin_time = fc::time_point::now();

        auto block_num = next_block.block_num();
        if (_checkpoints.size() && _checkpoints.rbegin()->second != block_id_type())
        {
            auto itr = _checkpoints.find(block_num);
            if (itr != _checkpoints.end())
                FC_ASSERT(next_block.id() == itr->second, "Block did not match checkpoint",
                          ("checkpoint", *itr)("block_id", next_block.id()));

            if (_checkpoints.rbegin()->first >= block_num)
                skip = skip_witness_signature | skip_transaction_signatures | skip_transaction_dupe_check | skip_fork_db
                    | skip_block_size_check | skip_tapos_check | skip_authority_check
                    /* | skip_merkle_check While blockchain is being downloaded, txs need to be validated against block
                       headers */
                    | skip_undo_history_check | skip_witness_schedule_check | skip_validate | skip_validate_invariants;
        }

        detail::with_skip_flags(*this, skip, [&]() { _apply_block(next_block); });

        /// check invariants
        if (is_producing() || !(skip & skip_validate_invariants))
        {
            try
            {
                validate_invariants();
            }
#ifdef DEBUG
            FC_CAPTURE_AND_RETHROW((next_block));
#else
            FC_CAPTURE_AND_LOG((next_block));
#endif
        }

        // fc::time_point end_time = fc::time_point::now();
        // fc::microseconds dt = end_time - begin_time;
        if (_flush_blocks != 0)
        {
            if (_next_flush_block == 0)
            {
                uint32_t lep = block_num + 1 + _flush_blocks * 9 / 10;
                uint32_t rep = block_num + 1 + _flush_blocks;

                // use time_point::now() as RNG source to pick block randomly between lep and rep
                uint32_t span = rep - lep;
                uint32_t x = lep;
                if (span > 0)
                {
                    uint64_t now = uint64_t(fc::time_point::now().time_since_epoch().count());
                    x += now % span;
                }
                _next_flush_block = x;
                // ilog( "Next flush scheduled at block ${b}", ("b", x) );
            }

            if (_next_flush_block == block_num)
            {
                _next_flush_block = 0;
                // ilog( "Flushing database shared memory at block ${b}", ("b", block_num) );
                chainbase::database::flush();
            }
        }

        show_free_memory(false);
    }
    FC_CAPTURE_AND_RETHROW((next_block))
}

void database::show_free_memory(bool force)
{
    uint32_t free_gb = uint32_t(get_free_memory() / (1024 * 1024 * 1024));
    if (force || (free_gb < _last_free_gb_printed) || (free_gb > _last_free_gb_printed + 1))
    {
        ilog("Free memory is now ${n}G", ("n", free_gb));
        _last_free_gb_printed = free_gb;
    }

    if (free_gb == 0)
    {
        uint32_t free_mb = uint32_t(get_free_memory() / (1024 * 1024));

        if (free_mb <= SCORUM_DB_FREE_MEMORY_THRESHOLD_MB && head_block_num() % 10 == 0)
        {
            elog("Free memory is now ${n}M. Increase shared file size immediately!", ("n", free_mb));
        }
    }
}

void database::_apply_block(const signed_block& next_block)
{
    try
    {
        notify_pre_applied_block(next_block);

        uint32_t next_block_num = next_block.block_num();
        // block_id_type next_block_id = next_block.id();

        uint32_t skip = get_node_properties().skip_flags;

        if (!(skip & skip_merkle_check))
        {
            auto merkle_root = next_block.calculate_merkle_root();

            try
            {
                FC_ASSERT(next_block.transaction_merkle_root == merkle_root, "Merkle check failed",
                          ("next_block.transaction_merkle_root", next_block.transaction_merkle_root)(
                              "calc", merkle_root)("next_block", next_block)("id", next_block.id()));
            }
            catch (fc::assert_exception& e)
            {
                const auto& merkle_map = get_shared_db_merkle();
                auto itr = merkle_map.find(next_block_num);

                if (itr == merkle_map.end() || itr->second != merkle_root)
                {
                    throw e;
                }
            }
        }

        const witness_object& signing_witness = validate_block_header(skip, next_block);

        _current_block_num = next_block_num;
        _current_trx_in_block = 0;

        const auto& gprops = obtain_service<dbs_dynamic_global_property>().get();
        auto block_size = fc::raw::pack_size(next_block);
        FC_ASSERT(block_size <= gprops.median_chain_props.maximum_block_size, "Block Size is too Big",
                  ("next_block_num", next_block_num)("block_size",
                                                     block_size)("max", gprops.median_chain_props.maximum_block_size));

        /// modify current witness so transaction evaluators can know who included the transaction,
        /// this is mostly for POW operations which must pay the current_witness
        modify(gprops, [&](dynamic_global_property_object& dgp) { dgp.current_witness = next_block.witness; });

        /// parse witness version reporting
        process_header_extensions(next_block);

        const auto& witness = obtain_service<dbs_witness>().get(next_block.witness);
        const auto& hardfork_state = obtain_service<dbs_hardfork_property>().get();
        FC_ASSERT(witness.running_version >= hardfork_state.current_hardfork_version,
                  "Block produced by witness that is not running current hardfork",
                  ("witness", witness)("next_block.witness", next_block.witness)("hardfork_state", hardfork_state));

        for (const auto& trx : next_block.transactions)
        {
            /* We do not need to push the undo state for each transaction
             * because they either all apply and are valid or the
             * entire block fails to apply.  We only need an "undo" state
             * for transactions when validating broadcast transactions or
             * when building a block.
             */
            apply_transaction(trx, skip);
            ++_current_trx_in_block;
        }

        update_global_dynamic_data(next_block);
        update_signing_witness(signing_witness, next_block);

        update_last_irreversible_block();

        create_block_summary(next_block);
        clear_expired_transactions();
        clear_expired_delegations();

        // in dbs_database_witness_schedule.cpp
        update_witness_schedule();

        database_ns::block_task_context ctx(static_cast<data_service_factory&>(*this),
                                            static_cast<database_virtual_operations_emmiter_i&>(*this),
                                            _current_block_num);

        _my->_process_funds.before(_my->_process_comments_cashout).before(_my->_process_vesting_withdrawals).apply(ctx);

        obtain_service<dbs_atomicswap>().check_contracts_expiration();

        account_recovery_processing();
        expire_escrow_ratification();
        process_decline_voting_rights();

        obtain_service<dbs_proposal>().clear_expired_proposals();

        process_hardforks();

        // notify observers that the block has been applied
        notify_applied_block(next_block);
    }
    FC_CAPTURE_LOG_AND_RETHROW((next_block.block_num()))
}

void database::process_header_extensions(const signed_block& next_block)
{
    auto& witness_service = obtain_service<dbs_witness>();

    auto itr = next_block.extensions.begin();

    while (itr != next_block.extensions.end())
    {
        switch (itr->which())
        {
        case 0: // void_t
            break;
        case 1: // version
        {
            auto reported_version = itr->get<version>();
            const auto& signing_witness = witness_service.get(next_block.witness);
            // idump( (next_block.witness)(signing_witness.running_version)(reported_version) );

            if (reported_version != signing_witness.running_version)
            {
                modify(signing_witness, [&](witness_object& wo) { wo.running_version = reported_version; });
            }
            break;
        }
        case 2: // hardfork_version vote
        {
            auto hfv = itr->get<hardfork_version_vote>();
            const auto& signing_witness = witness_service.get(next_block.witness);
            // idump( (next_block.witness)(signing_witness.running_version)(hfv) );

            if (hfv.hf_version != signing_witness.hardfork_version_vote
                || hfv.hf_time != signing_witness.hardfork_time_vote)
                modify(signing_witness, [&](witness_object& wo) {
                    wo.hardfork_version_vote = hfv.hf_version;
                    wo.hardfork_time_vote = hfv.hf_time;
                });

            break;
        }
        default:
            FC_ASSERT(false, "Unknown extension in block header");
        }

        ++itr;
    }
}

void database::apply_transaction(const signed_transaction& trx, uint32_t skip)
{
    detail::with_skip_flags(*this, skip, [&]() { _apply_transaction(trx); });
    notify_on_applied_transaction(trx);
}

void database::_apply_transaction(const signed_transaction& trx)
{
    try
    {
        _current_trx_id = trx.id();
        uint32_t skip = get_node_properties().skip_flags;

        if (!(skip & skip_validate)) /* issue #505 explains why this skip_flag is disabled */
        {
            trx.validate();
        }

        auto& trx_idx = get_index<transaction_index>();
        auto trx_id = trx.id();
        // idump((trx_id)(skip&skip_transaction_dupe_check));
        FC_ASSERT((skip & skip_transaction_dupe_check)
                      || trx_idx.indices().get<by_trx_id>().find(trx_id) == trx_idx.indices().get<by_trx_id>().end(),
                  "Duplicate transaction check failed", ("trx_ix", trx_id));

        if (!(skip & (skip_transaction_signatures | skip_authority_check)))
        {
            auto get_active = [&](const std::string& name) {
                return authority(get<account_authority_object, by_account>(name).active);
            };
            auto get_owner = [&](const std::string& name) {
                return authority(get<account_authority_object, by_account>(name).owner);
            };
            auto get_posting = [&](const std::string& name) {
                return authority(get<account_authority_object, by_account>(name).posting);
            };

            try
            {
                trx.verify_authority(get_chain_id(), get_active, get_owner, get_posting, SCORUM_MAX_SIG_CHECK_DEPTH);
            }
            catch (protocol::tx_missing_active_auth& e)
            {
                if (get_shared_db_merkle().find(head_block_num() + 1) == get_shared_db_merkle().end())
                {
                    throw e;
                }
            }
        }

        // Skip all manner of expiration and TaPoS checking if we're on block 1; It's impossible that the transaction is
        // expired, and TaPoS makes no sense as no blocks exist.
        if (BOOST_LIKELY(head_block_num() > 0))
        {
            if (!(skip & skip_tapos_check))
            {
                const auto& tapos_block_summary = get<block_summary_object>(trx.ref_block_num);
                // Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the
                // expiration
                SCORUM_ASSERT(trx.ref_block_prefix == tapos_block_summary.block_id._hash[1],
                              transaction_tapos_exception, "",
                              ("trx.ref_block_prefix", trx.ref_block_prefix)("tapos_block_summary",
                                                                             tapos_block_summary.block_id._hash[1]));
            }

            fc::time_point_sec now = head_block_time();

            SCORUM_ASSERT(
                trx.expiration <= now + fc::seconds(SCORUM_MAX_TIME_UNTIL_EXPIRATION), transaction_expiration_exception,
                "", ("trx.expiration", trx.expiration)("now", now)("max_til_exp", SCORUM_MAX_TIME_UNTIL_EXPIRATION));
            // Simple solution to pending trx bug when now == trx.expiration
            SCORUM_ASSERT(now < trx.expiration, transaction_expiration_exception, "",
                          ("now", now)("trx.exp", trx.expiration));
        }

        // Insert transaction into unique transactions database.
        if (!(skip & skip_transaction_dupe_check))
        {
            create<transaction_object>([&](transaction_object& transaction) {
                transaction.trx_id = trx_id;
                transaction.expiration = trx.expiration;
                fc::raw::pack(transaction.packed_trx, trx);
            });
        }

        notify_on_pre_apply_transaction(trx);

        // Finally process the operations
        _current_op_in_trx = 0;
        for (const auto& op : trx.operations)
        {
            try
            {
                apply_operation(op);
                ++_current_op_in_trx;
            }
            FC_CAPTURE_AND_RETHROW((op));
        }
        _current_trx_id = transaction_id_type();
    }
    FC_CAPTURE_AND_RETHROW((trx))
}

void database::apply_operation(const operation& op)
{
    operation_notification note(op);
    notify_pre_apply_operation(note);
    _my->_evaluator_registry.get_evaluator(op).apply(op);
    notify_post_apply_operation(note);
}

const witness_object& database::validate_block_header(uint32_t skip, const signed_block& next_block) const
{
    try
    {
        FC_ASSERT(head_block_id() == next_block.previous, "",
                  ("head_block_id", head_block_id())("next.prev", next_block.previous));
        FC_ASSERT(
            head_block_time() < next_block.timestamp, "",
            ("head_block_time", head_block_time())("next", next_block.timestamp)("blocknum", next_block.block_num()));

        const witness_object& witness = obtain_service<dbs_witness>().get(next_block.witness);

        if (!(skip & skip_witness_signature))
        {
            FC_ASSERT(next_block.validate_signee(witness.signing_key));
        }

        if (!(skip & skip_witness_schedule_check))
        {
            uint32_t slot_num = get_slot_at_time(next_block.timestamp);
            FC_ASSERT(slot_num > 0);

            std::string scheduled_witness = get_scheduled_witness(slot_num);

            FC_ASSERT(witness.owner == scheduled_witness, "Witness produced block at wrong time",
                      ("block witness", next_block.witness)("scheduled", scheduled_witness)("slot_num", slot_num));
        }

        return witness;
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::create_block_summary(const signed_block& next_block)
{
    try
    {
        block_summary_id_type sid(next_block.block_num() & (uint32_t)SCORUM_BLOCKID_POOL_SIZE);
        modify(get<block_summary_object>(sid), [&](block_summary_object& p) { p.block_id = next_block.id(); });
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::update_global_dynamic_data(const signed_block& b)
{
    try
    {
        const dynamic_global_property_object& _dgp = obtain_service<dbs_dynamic_global_property>().get();
        auto& witness_service = obtain_service<dbs_witness>();

        uint32_t missed_blocks = 0;
        if (head_block_time() != fc::time_point_sec())
        {
            missed_blocks = get_slot_at_time(b.timestamp);
            assert(missed_blocks != 0);
            missed_blocks--;
            for (uint32_t i = 0; i < missed_blocks; ++i)
            {
                const auto& witness_missed = witness_service.get(get_scheduled_witness(i + 1));
                if (witness_missed.owner != b.witness)
                {
                    modify(witness_missed, [&](witness_object& w) {
                        w.total_missed++;

                        push_virtual_operation(witness_miss_block_operation(w.owner, b.block_num()));

                        if (head_block_num() - w.last_confirmed_block_num > SCORUM_WITNESS_MISSED_BLOCKS_THRESHOLD)
                        {
                            w.signing_key = public_key_type();
                            push_virtual_operation(shutdown_witness_operation(w.owner));
                        }
                    });
                }
            }
        }

        // dynamic global properties updating
        modify(_dgp, [&](dynamic_global_property_object& dgp) {
            // This is constant time assuming 100% participation. It is O(B) otherwise (B = Num blocks between update)
            for (uint32_t i = 0; i < missed_blocks + 1; i++)
            {
                dgp.participation_count -= dgp.recent_slots_filled.hi & 0x8000000000000000ULL ? 1 : 0;
                dgp.recent_slots_filled = (dgp.recent_slots_filled << 1) + (i == 0 ? 1 : 0);
                dgp.participation_count += (i == 0 ? 1 : 0);
            }

            dgp.head_block_number = b.block_num();
            dgp.head_block_id = b.id();
            dgp.time = b.timestamp;
            dgp.current_aslot += missed_blocks + 1;
        });

        if (!(get_node_properties().skip_flags & skip_undo_history_check))
        {
            SCORUM_ASSERT(
                _dgp.head_block_number - _dgp.last_irreversible_block_num < SCORUM_MAX_UNDO_HISTORY,
                undo_database_exception,
                "The database does not have enough undo history to support a blockchain with so many missed blocks. "
                "Please add a checkpoint if you would like to continue applying blocks beyond this point.",
                ("last_irreversible_block_num", _dgp.last_irreversible_block_num)("head", _dgp.head_block_number)(
                    "max_undo", SCORUM_MAX_UNDO_HISTORY));
        }
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::update_signing_witness(const witness_object& signing_witness, const signed_block& new_block)
{
    try
    {
        modify(signing_witness, [&](witness_object& _wit) { _wit.last_confirmed_block_num = new_block.block_num(); });
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::update_last_irreversible_block()
{
    try
    {
        const dynamic_global_property_object& dpo = obtain_service<dbs_dynamic_global_property>().get();

        /**
         * Prior to voting taking over, we must be more conservative...
         *
         */
        if (head_block_num() < SCORUM_START_MINER_VOTING_BLOCK)
        {
            modify(dpo, [&](dynamic_global_property_object& _dpo) {
                if (head_block_num() > SCORUM_MAX_WITNESSES)
                {
                    _dpo.last_irreversible_block_num = head_block_num() - SCORUM_MAX_WITNESSES;
                }
            });
        }
        else
        {
            auto& witness_service = obtain_service<dbs_witness>();
            const witness_schedule_object& wso = obtain_service<dbs_witness_schedule>().get();

            std::vector<const witness_object*> wit_objs;
            wit_objs.reserve(wso.num_scheduled_witnesses);
            for (int i = 0; i < wso.num_scheduled_witnesses; i++)
            {
                wit_objs.push_back(&witness_service.get(wso.current_shuffled_witnesses[i]));
            }

            static_assert(SCORUM_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero");

            // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
            // 1 1 1 1 1 1 1 2 2 2 -> 1
            // 3 3 3 3 3 3 3 3 3 3 -> 3

            size_t offset
                = ((SCORUM_100_PERCENT - SCORUM_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / SCORUM_100_PERCENT);

            std::nth_element(wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
                             [](const witness_object* a, const witness_object* b) {
                                 return a->last_confirmed_block_num < b->last_confirmed_block_num;
                             });

            uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;

            if (new_last_irreversible_block_num > dpo.last_irreversible_block_num)
            {
                modify(dpo, [&](dynamic_global_property_object& _dpo) {
                    _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
                });
            }
        }

        for_each_index(
            [&](chainbase::abstract_generic_index_i& item) { item.commit(dpo.last_irreversible_block_num); });

        if (!(get_node_properties().skip_flags & skip_block_log))
        {
            // output to block log based on new last irreversible block num
            const auto& tmp_head = _block_log.head();
            uint64_t log_head_num = 0;

            if (tmp_head)
            {
                log_head_num = tmp_head->block_num();
            }

            if (log_head_num < dpo.last_irreversible_block_num)
            {
                while (log_head_num < dpo.last_irreversible_block_num)
                {
                    std::shared_ptr<fork_item> block = _fork_db.fetch_block_on_main_branch_by_number(log_head_num + 1);
                    FC_ASSERT(block, "Current fork in the fork database does not contain the last_irreversible_block");
                    _block_log.append(block->data);
                    log_head_num++;
                }

                _block_log.flush();
            }
        }

        _fork_db.set_max_size(dpo.head_block_number - dpo.last_irreversible_block_num + 1);
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::clear_expired_transactions()
{
    // Look for expired transactions in the deduplication list, and remove them.
    // Transactions must have expired by at least two forking windows in order to be removed.
    auto& transaction_idx = get_index<transaction_index>();
    const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
    while ((!dedupe_index.empty()) && (head_block_time() > dedupe_index.begin()->expiration))
    {
        remove(*dedupe_index.begin());
    }
}

void database::clear_expired_delegations()
{
    auto now = head_block_time();
    const auto& delegations_by_exp = get_index<scorumpower_delegation_expiration_index, by_expiration>();
    const auto& account_service = obtain_service<dbs_account>();
    auto itr = delegations_by_exp.begin();
    while (itr != delegations_by_exp.end() && itr->expiration < now)
    {
        modify(account_service.get_account(itr->delegator),
               [&](account_object& a) { a.delegated_scorumpower -= itr->scorumpower; });

        push_virtual_operation(return_scorumpower_delegation_operation(itr->delegator, itr->scorumpower));

        remove(*itr);
        itr = delegations_by_exp.begin();
    }
}

const genesis_persistent_state_type& database::genesis_persistent_state() const
{
    return _my->_genesis_persistent_state;
}

void database::adjust_balance(const account_object& a, const asset& delta)
{
    modify(a, [&](account_object& acnt) {
        switch (delta.symbol())
        {
        case SCORUM_SYMBOL:
            acnt.balance += delta;
            break;
        default:
            FC_ASSERT(false, "invalid symbol");
        }
    });
}

void database::init_hardforks(time_point_sec genesis_time)
{
    _hardfork_times[0] = genesis_time;
    _hardfork_versions[0] = hardfork_version(0, 0);

    // SCORUM: structure to initialize hardofrks

    // FC_ASSERT( SCORUM_HARDFORK_0_1 == 1, "Invalid hardfork configuration" );
    //_hardfork_times[ SCORUM_HARDFORK_0_1 ] = fc::time_point_sec( SCORUM_HARDFORK_0_1_TIME );
    //_hardfork_versions[ SCORUM_HARDFORK_0_1 ] = SCORUM_HARDFORK_0_1_VERSION;

    const auto& hardforks = obtain_service<dbs_hardfork_property>().get();
    FC_ASSERT(hardforks.last_hardfork <= SCORUM_NUM_HARDFORKS, "Chain knows of more hardforks than configuration",
              ("hardforks.last_hardfork", hardforks.last_hardfork)("SCORUM_NUM_HARDFORKS", SCORUM_NUM_HARDFORKS));
    FC_ASSERT(_hardfork_versions[hardforks.last_hardfork] <= SCORUM_BLOCKCHAIN_VERSION,
              "Blockchain version is older than last applied hardfork");
    FC_ASSERT(SCORUM_BLOCKCHAIN_HARDFORK_VERSION == _hardfork_versions[SCORUM_NUM_HARDFORKS]);
}

void database::process_hardforks()
{
    try
    {
        // If there are upcoming hardforks and the next one is later, do nothing
        const auto& hardforks = obtain_service<dbs_hardfork_property>().get();

        while (_hardfork_versions[hardforks.last_hardfork] < hardforks.next_hardfork
               && hardforks.next_hardfork_time <= head_block_time())
        {
            if (hardforks.last_hardfork < SCORUM_NUM_HARDFORKS)
            {
                apply_hardfork(hardforks.last_hardfork + 1);
            }
            else
            {
                throw unknown_hardfork_exception();
            }
        }
    }
    FC_CAPTURE_AND_RETHROW()
}

bool database::has_hardfork(uint32_t hardfork) const
{
    return obtain_service<dbs_hardfork_property>().get().processed_hardforks.size() > hardfork;
}

void database::set_hardfork(uint32_t hardfork, bool apply_now)
{
    auto const& hardforks = obtain_service<dbs_hardfork_property>().get();

    for (uint32_t i = hardforks.last_hardfork + 1; i <= hardfork && i <= SCORUM_NUM_HARDFORKS; i++)
    {
        modify(hardforks, [&](hardfork_property_object& hpo) {
            hpo.next_hardfork = _hardfork_versions[i];
            hpo.next_hardfork_time = head_block_time();
        });

        if (apply_now)
        {
            apply_hardfork(i);
        }
    }
}

void database::apply_hardfork(uint32_t hardfork)
{
    if (_options & opt_log_hardforks)
    {
        elog("HARDFORK ${hf} at block ${b}", ("hf", hardfork)("b", head_block_num()));
    }

    switch (hardfork)
    {
    default:
        break;
    }

    obtain_service<dbs_hardfork_property>().update([&](hardfork_property_object& hfp) {
        FC_ASSERT(hardfork == hfp.last_hardfork + 1, "Hardfork being applied out of order",
                  ("hardfork", hardfork)("hfp.last_hardfork", hfp.last_hardfork));
        FC_ASSERT(hfp.processed_hardforks.size() == hardfork, "Hardfork being applied out of order");
        hfp.processed_hardforks.push_back(_hardfork_times[hardfork]);
        hfp.last_hardfork = hardfork;
        hfp.current_hardfork_version = _hardfork_versions[hardfork];
        FC_ASSERT(hfp.processed_hardforks[hfp.last_hardfork] == _hardfork_times[hfp.last_hardfork],
                  "Hardfork processing failed sanity check...");
    });

    push_hf_operation(hardfork_operation(hardfork));
}

/**
 * Verifies all supply invariants check out
 */
void database::validate_invariants() const
{
    try
    {
        asset total_supply = asset(0, SCORUM_SYMBOL);
        asset total_scorumpower = asset(0, SP_SYMBOL);
        share_type total_vsf_votes = share_type(0);

        const auto& gpo = obtain_service<dbs_dynamic_global_property>().get();

        /// verify no witness has too many votes
        const auto& witness_idx = get_index<witness_index>().indices();
        for (auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr)
        {
            FC_ASSERT(itr->votes <= gpo.total_scorumpower.amount, "${vs} > ${tvs}",
                      ("vs", itr->votes)("tvs", gpo.total_scorumpower.amount));
        }

        const auto& account_idx = get_index<account_index>().indices().get<by_name>();
        for (auto itr = account_idx.begin(); itr != account_idx.end(); ++itr)
        {
            total_supply += itr->balance;
            total_scorumpower += itr->scorumpower;
            total_vsf_votes += (itr->proxy == SCORUM_PROXY_TO_SELF_ACCOUNT
                                    ? itr->witness_vote_weight()
                                    : (SCORUM_MAX_PROXY_RECURSION_DEPTH > 0
                                           ? itr->proxied_vsf_votes[SCORUM_MAX_PROXY_RECURSION_DEPTH - 1]
                                           : itr->scorumpower.amount));
        }

        const auto& escrow_idx = get_index<escrow_index>().indices().get<by_id>();
        for (auto itr = escrow_idx.begin(); itr != escrow_idx.end(); ++itr)
        {
            total_supply += itr->scorum_balance;
            total_supply += itr->pending_fee;
        }

        total_supply += obtain_service<dbs_reward_fund>().get().activity_reward_balance_scr;
        total_supply += asset(gpo.total_scorumpower.amount, SCORUM_SYMBOL);
        total_supply += obtain_service<dbs_reward>().get().balance;

        for (const budget_object& budget : obtain_service<dbs_budget>().get_budgets())
        {
            total_supply += budget.balance;
        }

        if (obtain_service<dbs_budget>().is_fund_budget_exists())
        {
            total_supply += obtain_service<dbs_budget>().get_fund_budget().balance.amount;
        }

        if (obtain_service<dbs_registration_pool>().is_exists())
        {
            total_supply += obtain_service<dbs_registration_pool>().get().balance;
        }

        total_supply += asset(obtain_service<dbs_dev_pool>().get().sp_balance.amount, SCORUM_SYMBOL);
        total_supply += obtain_service<dbs_dev_pool>().get().scr_balance;

        const auto& atomicswap_contract_idx = get_index<atomicswap_contract_index, by_id>();
        for (auto itr = atomicswap_contract_idx.begin(); itr != atomicswap_contract_idx.end(); ++itr)
        {
            total_supply += itr->amount;
        }

        FC_ASSERT(total_supply <= asset::maximum(SCORUM_SYMBOL), "Assets SCR overflow");
        FC_ASSERT(total_scorumpower <= asset::maximum(SP_SYMBOL), "Assets SP overflow");

        FC_ASSERT(gpo.total_supply == total_supply, "",
                  ("gpo.total_supply", gpo.total_supply)("total_supply", total_supply));
        FC_ASSERT(gpo.total_scorumpower == total_scorumpower, "",
                  ("gpo.total_scorumpower", gpo.total_scorumpower)("total_scorumpower", total_scorumpower));
        FC_ASSERT(gpo.total_scorumpower.amount == total_vsf_votes, "",
                  ("total_scorumpower", gpo.total_scorumpower)("total_vsf_votes", total_vsf_votes));
    }
    FC_CAPTURE_LOG_AND_RETHROW((head_block_num()));
}

} // namespace chain
} // namespace scorum
