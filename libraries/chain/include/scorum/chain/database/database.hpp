/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 */
#pragma once

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/hardfork.hpp>
#include <scorum/chain/node_property_object.hpp>
#include <scorum/chain/database/fork_database.hpp>
#include <scorum/chain/block_log.hpp>
#include <scorum/chain/operation_notification.hpp>

#include <scorum/protocol/protocol.hpp>

#include <scorum/chain/dba/db_accessor_factory.hpp>
#include <scorum/chain/services/dbservice_dbs_factory.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/database/database_virtual_operations.hpp>

#include <fc/signals.hpp>
#include <fc/shared_string.hpp>
#include <fc/log/logger.hpp>

#include <map>
#include <memory>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::asset_symbol_type;
using scorum::protocol::authority;
using scorum::protocol::operation;
using scorum::protocol::signed_transaction;

class database_impl;

struct genesis_state_type;
struct genesis_persistent_state_type;

/**
 *   @class database
 *   @brief tracks the blockchain state in an extensible manner
 */
class database : public chainbase::database,
                 public dbservice_dbs_factory,
                 public data_service_factory,
                 public dba::db_accessor_factory,
                 public database_virtual_operations_emmiter_i
{

public:
    enum creation_option
    {
        opt_none = 0,
        opt_log_hardforks = 1 << 0,
        opt_notify_virtual_op_applying = 1 << 1,

#ifdef IS_LOW_MEM
        opt_default = opt_log_hardforks
#else
        opt_default = opt_log_hardforks | opt_notify_virtual_op_applying
#endif
    };

    database(uint32_t opt);
    virtual ~database();

    bool is_producing() const
    {
        return _is_producing;
    }

    enum validation_steps
    {
        skip_nothing = 0,
        skip_witness_signature = 1 << 0, ///< used while reindexing
        skip_transaction_signatures = 1 << 1, ///< used by non-witness nodes
        skip_transaction_dupe_check = 1 << 2, ///< used while reindexing
        skip_fork_db = 1 << 3, ///< used while reindexing
        skip_block_size_check = 1 << 4, ///< used when applying locally generated transactions
        skip_tapos_check = 1 << 5, ///< used while reindexing -- note this skips expiration check as well
        skip_authority_check = 1 << 6, ///< used while reindexing -- disables any checking of authority on transactions
        skip_merkle_check = 1 << 7, ///< used while reindexing
        skip_undo_history_check = 1 << 8, ///< used while reindexing
        skip_witness_schedule_check = 1 << 9, ///< used while reindexing
        skip_validate = 1 << 10, ///< used prior to checkpoint, skips validate() call on transaction
        skip_validate_invariants = 1 << 11, ///< used to skip database invariant check on block application
        skip_undo_block = 1 << 12, ///< used to skip undo db on reindex
        skip_block_log = 1 << 13 ///< used to skip block logging on reindex
    };

    static fc::path block_log_path(const fc::path& data_dir);

    uint32_t get_reindex_skip_flags() const;

    /**
     * @brief Open a database, creating a new one if necessary
     *
     * Opens a database in the specified directory. If no initialized database is found the database
     * will be initialized with the default state.
     *
     * @param data_dir Path to open or create database in
     */
    void open(const fc::path& data_dir,
              const fc::path& shared_mem_dir,
              uint64_t shared_file_size,
              uint32_t chainbase_flags,
              const genesis_state_type& genesis_state);

    /**
     * @brief Rebuild object graph from block history and open detabase
     *
     * This method may be called after or instead of @ref database::open, and will rebuild the object graph by
     * replaying blockchain history. When this method exits successfully, the database will be open.
     */
    void reindex(const fc::path& data_dir,
                 const fc::path& shared_mem_dir,
                 uint64_t shared_file_size,
                 uint32_t skip_flags,
                 const genesis_state_type& genesis_state);

    /**
     * @brief wipe Delete database from disk, and potentially the raw chain as well.
     * @param include_blocks If true, delete the raw chain as well as the database.
     *
     * Will close the database before wiping. Database will be closed when this function returns.
     */
    void wipe(const fc::path& data_dir, const fc::path& shared_mem_dir, bool include_blocks);

    void close();

    time_point_sec get_genesis_time() const;

    //////////////////// db_block.cpp ////////////////////

    /**
     *  @return true if the block is in our fork DB or saved to disk as
     *  part of the official chain, otherwise return false
     */
    bool is_known_block(const block_id_type& id) const;
    bool is_known_transaction(const transaction_id_type& id) const;
    block_id_type find_block_id_for_num(uint32_t block_num) const;
    block_id_type get_block_id_for_num(uint32_t block_num) const;
    optional<signed_block> fetch_block_by_id(const block_id_type& id) const;
    optional<signed_block> fetch_block_by_number(uint32_t num) const;
    optional<signed_block> read_block_by_number(uint32_t num) const;

    const signed_transaction get_recent_transaction(const transaction_id_type& trx_id) const;
    std::vector<block_id_type> get_block_ids_on_fork(block_id_type head_of_fork) const;

    chain_id_type get_chain_id() const;

    const node_property_object& get_node_properties() const;

    const time_point_sec calculate_discussion_payout_time(const comment_object& comment) const;

    /**
     *  Calculate the percent of block production slots that were missed in the
     *  past 128 blocks, not including the current block.
     */
    uint32_t witness_participation_rate() const;

    void add_checkpoints(const flat_map<uint32_t, block_id_type>& checkpts);
    const flat_map<uint32_t, block_id_type> get_checkpoints() const
    {
        return _checkpoints;
    }
    bool before_last_checkpoint() const;

    bool push_block(const signed_block& b, uint32_t skip = skip_nothing);
    void push_transaction(const signed_transaction& trx, uint32_t skip = skip_nothing);

    void _push_transaction(const signed_transaction& trx);

    signed_block generate_block(const fc::time_point_sec when,
                                const account_name_type& witness_owner,
                                const fc::ecc::private_key& block_signing_private_key,
                                uint32_t skip);

    void pop_block();
    void clear_pending();

    /**
     *  This method is used to track applied operations during the evaluation of a block, these
     *  operations should include any operation actually included in a transaction as well
     *  as any implied/virtual operations that resulted, such as filling an order.
     *  The applied operations are cleared after post_apply_operation.
     */
    void notify_pre_apply_operation(const operation_notification& note);
    void notify_post_apply_operation(const operation_notification& note);
    operation_notification create_notification(const operation& op) const;

    // vops are not needed for low mem. Force will push them on low mem.
    inline void push_virtual_operation(const operation& op);
    inline void push_hf_operation(const operation& op);

    void notify_pre_applied_block(const signed_block& block);
    void notify_applied_block(const signed_block& block);
    void notify_on_pending_transaction(const signed_transaction& tx);
    void notify_on_pre_apply_transaction(const signed_transaction& tx);
    void notify_on_applied_transaction(const signed_transaction& tx);

    /**
     *  This signal is emitted for plugins to process every operation before/after it has been fully applied.
     */
    fc::signal<void(const operation_notification&)> pre_apply_operation;
    fc::signal<void(const operation_notification&)> post_apply_operation;
    fc::signal<void(const signed_block&)> pre_applied_block;

    /**
     *  This signal is emitted after all operations and virtual operation for a
     *  block have been applied but before the get_applied_operations() are cleared.
     *
     *  You may not yield from this callback because the blockchain is holding
     *  the write lock and may be in an "inconstant state" until after it is
     *  released.
     */
    fc::signal<void(const signed_block&)> applied_block;

    /**
     * This signal is emitted any time a new transaction is added to the pending
     * block state.
     */
    fc::signal<void(const signed_transaction&)> on_pending_transaction;

    /**
     * This signla is emitted any time a new transaction is about to be applied
     * to the chain state.
     */
    fc::signal<void(const signed_transaction&)> on_pre_apply_transaction;

    /**
     * This signal is emitted any time a new transaction has been applied to the
     * chain state.
     */
    fc::signal<void(const signed_transaction&)> on_applied_transaction;

    //////////////////// db_witness_schedule.cpp ////////////////////

    /**
     * @brief Get the witness scheduled for block production in a slot.
     *
     * slot_num always corresponds to a time in the future.
     *
     * If slot_num == 1, returns the next scheduled witness.
     * If slot_num == 2, returns the next scheduled witness after
     * 1 block gap.
     *
     * Use the get_slot_time() and get_slot_at_time() functions
     * to convert between slot_num and timestamp.
     *
     * Passing slot_num == 0 returns SCORUM_NULL_WITNESS
     */
    account_name_type get_scheduled_witness(uint32_t slot_num) const;

    /**
     * Get the time at which the given slot occurs.
     *
     * If slot_num == 0, return time_point_sec().
     *
     * If slot_num == N for N > 0, return the Nth next
     * block-interval-aligned time greater than head_block_time().
     */
    fc::time_point_sec get_slot_time(uint32_t slot_num) const;

    /**
     * Get the last slot which occurs AT or BEFORE the given time.
     *
     * The return value is the greatest value N such that
     * get_slot_time( N ) <= when.
     *
     * If no such N exists, return 0.
     */
    uint32_t get_slot_at_time(fc::time_point_sec when) const;

    void account_recovery_processing();
    void expire_escrow_ratification();
    void process_decline_voting_rights();

    time_point_sec head_block_time() const;
    uint32_t head_block_num() const;
    block_id_type head_block_id() const;

    block_info head_block_context() const;

    node_property_object& node_properties();

    uint32_t last_non_undoable_block_num() const;
    //////////////////// db_init.cpp ////////////////////

    void initialize_evaluators();

    /// Reset the object graph in-memory
    void initialize_indexes();

    void init_genesis(const genesis_state_type& genesis_state);

    /**
     *  This method validates transactions without adding it to the pending state.
     *  @throw if an error occurs
     */
    void validate_transaction(const signed_transaction& trx);

    /** when popping a block, the transactions that were removed get cached here so they
     * can be reapplied at the proper time */
    std::deque<signed_transaction> _popped_tx;

    bool has_hardfork(uint32_t hardfork) const;

    /* For testing and debugging only. Given a hardfork
       with id N, applies all hardforks with id <= N */
    void set_hardfork(uint32_t hardfork, bool process_now = true);

    void validate_invariants() const;

    void set_flush_interval(uint32_t flush_blocks);
    void show_free_memory(bool force);

    // index

    template <typename MultiIndexType> void add_plugin_index()
    {
        _plugin_index_signal.connect([this]() { this->add_index<MultiIndexType>(); });
    }

    const genesis_persistent_state_type& genesis_persistent_state() const;

private:
    // witness_schedule
    void update_witness_schedule();
    void _reset_witness_virtual_schedule_time();
    void _update_witness_median_props();
    void _update_witness_majority_version();
    void _update_witness_hardfork_version_votes();

    void _maybe_warn_multiple_production(uint32_t height) const;
    bool _push_block(const signed_block& b);

    signed_block _generate_block(const fc::time_point_sec when,
                                 const account_name_type& witness_owner,
                                 const fc::ecc::private_key& block_signing_private_key);

protected:
    void set_producing(bool p)
    {
        _is_producing = p;
    }

    void apply_block(const signed_block& next_block, uint32_t skip = skip_nothing);
    void apply_transaction(const signed_transaction& trx, uint32_t skip = skip_nothing);
    void _apply_block(const signed_block& next_block);
    void _apply_transaction(const signed_transaction& trx);
    void apply_operation(const operation& op);

    /// Steps involved in applying a new block
    ///@{

    const witness_object& validate_block_header(uint32_t skip, const signed_block& next_block) const;
    void create_block_summary(const signed_block& next_block);

    void update_global_dynamic_data(const signed_block& b);
    void update_signing_witness(const witness_object& signing_witness, const signed_block& new_block);
    void update_last_irreversible_block();
    void clear_expired_transactions();
    void clear_expired_delegations();
    void process_header_extensions(const signed_block& next_block);

    void init_hardforks(fc::time_point_sec genesis_time);
    void process_hardforks();
    void apply_hardfork(uint32_t hardfork);
    ///@}

private:
    std::unique_ptr<database_impl> _my;

    bool _is_producing = false;
    uint32_t _options;

    optional<chainbase::abstract_undo_session_ptr> _pending_tx_session;

    std::vector<signed_transaction> _pending_tx;
    fork_database _fork_db;
    fc::time_point_sec _hardfork_times[SCORUM_NUM_HARDFORKS + 1];
    protocol::hardfork_version _hardfork_versions[SCORUM_NUM_HARDFORKS + 1];

    block_log _block_log;

    fc::signal<void()> _plugin_index_signal;

    transaction_id_type _current_trx_id;
    uint32_t _current_block_num = 0;
    uint16_t _current_trx_in_block = 0;
    uint16_t _current_op_in_trx = 0;

    flat_map<uint32_t, block_id_type> _checkpoints;

    node_property_object _node_property_object;

    uint32_t _flush_blocks = 0;
    uint32_t _next_flush_block = 0;

    uint32_t _last_free_gb_printed = 0;

    fc::time_point_sec _const_genesis_time; // should be const
};
} // namespace chain
} // namespace scorum

#define debug_log(CTX, FORMAT, ...) fc_ctx_dlog(fc::logger::get("debug"), CTX, FORMAT, __VA_ARGS__)
