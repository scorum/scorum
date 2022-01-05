#pragma once

#define API_DATABASE "database_api"

#include <scorum/app/state.hpp>

#include <scorum/chain/database/database.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/scorum_object_types.hpp>

#include <scorum/app/scorum_api_objects.hpp>

#include <scorum/tags/tags_plugin.hpp>

#include <scorum/witness/witness_plugin.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace scorum {
namespace app {

using namespace scorum::chain;
using namespace scorum::protocol;

struct api_context;

struct withdraw_route
{
    std::string from_account;
    std::string to_account;
    uint16_t percent;
    bool auto_vest;
};

enum withdraw_route_type
{
    incoming,
    outgoing,
    all
};

class database_api_impl;

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
class database_api
{
public:
    database_api(const scorum::app::api_context& ctx);
    ~database_api();

    ///////////////////
    // Subscriptions //
    ///////////////////

    void set_block_applied_callback(std::function<void(const variant& block_header)> cb);

    std::vector<account_name_type> get_active_witnesses() const;

    /////////////
    // Globals //
    /////////////

    /**
     * @brief Retrieve compile-time constants
     */
    fc::variant_object get_config() const;

    /**
     * @brief Get the chain ID
     */
    chain_id_type get_chain_id() const;

    /**
     * @brief Retrieve the current @ref dynamic_global_property_object
     */
    dynamic_global_property_api_obj get_dynamic_global_properties() const;
    witness_schedule_api_obj get_witness_schedule() const;

    //////////
    // Keys //
    //////////

    std::vector<std::set<std::string>> get_key_references(std::vector<public_key_type> key) const;

    //////////////
    // Accounts //
    //////////////

    std::vector<extended_account> get_accounts(const std::vector<std::string>& names) const;

    /**
     *  @return all accounts that refer to the key or account id in their owner or active authorities.
     */
    std::vector<account_id_type> get_account_references(account_id_type account_id) const;

    /**
     * @brief Get a list of accounts by name
     * @param account_names Names of the accounts to retrieve
     * @return The accounts holding the provided names
     *
     * This function has semantics identical to @ref get_objects
     */
    std::vector<optional<account_api_obj>> lookup_account_names(const std::vector<std::string>& account_names) const;

    /**
     * @brief Get names and IDs for registered accounts
     * @param lower_bound_name Lower bound of the first name to return
     * @param limit Maximum number of results to return -- must not exceed 1000
     * @return Map of account names to corresponding IDs
     */
    std::set<std::string> lookup_accounts(const std::string& lower_bound_name, uint32_t limit) const;

    /**
     * @brief Get the total number of accounts registered with the blockchain
     */
    uint64_t get_account_count() const;

    std::vector<budget_api_obj> get_budgets(const budget_type, const std::set<std::string>& account_names) const;

    std::set<std::string>
    lookup_budget_owners(const budget_type, const std::string& lower_bound_name, uint32_t limit) const;

    std::vector<atomicswap_contract_api_obj> get_atomicswap_contracts(const std::string& owner) const;

    atomicswap_contract_info_api_obj
    get_atomicswap_contract(const std::string& from, const std::string& to, const std::string& secret_hash) const;

    std::vector<owner_authority_history_api_obj> get_owner_history(const std::string& account) const;

    optional<account_recovery_request_api_obj> get_recovery_request(const std::string& account) const;

    optional<escrow_api_obj> get_escrow(const std::string& from, uint32_t escrow_id) const;

    std::vector<withdraw_route> get_withdraw_routes(const std::string& account,
                                                    withdraw_route_type type = outgoing) const;

    optional<account_bandwidth_api_obj> get_account_bandwidth(const std::string& account,
                                                              witness::bandwidth_type type) const;

    std::vector<scorumpower_delegation_api_obj>
    get_scorumpower_delegations(const std::string& account, const std::string& from, uint32_t limit = 100) const;
    std::vector<scorumpower_delegation_expiration_api_obj>
    get_expiring_scorumpower_delegations(const std::string& account, time_point_sec from, uint32_t limit = 100) const;

    ///////////////
    // Witnesses //
    ///////////////

    /**
     * @brief Get a list of witnesses by ID
     * @param witness_ids IDs of the witnesses to retrieve
     * @return The witnesses corresponding to the provided IDs
     *
     * This function has semantics identical to @ref get_objects
     */
    std::vector<optional<witness_api_obj>> get_witnesses(const std::vector<witness_id_type>& witness_ids) const;

    /**
     * @brief Get the witness owned by a given account
     * @param account The name of the account whose witness should be retrieved
     * @return The witness object, or null if the account does not have a witness
     */
    fc::optional<witness_api_obj> get_witness_by_account(const std::string& account_name) const;

    /**
     *  This method is used to fetch witnesses with pagination.
     *
     *  @return an array of `count` witnesses sorted by total votes after witness `from` with at most `limit' results.
     */
    std::vector<witness_api_obj> get_witnesses_by_vote(const std::string& from, uint32_t limit) const;

    /**
     * @brief Get names and IDs for registered witnesses
     * @param lower_bound_name Lower bound of the first name to return
     * @param limit Maximum number of results to return -- must not exceed 1000
     * @return Map of witness names to corresponding IDs
     */
    std::set<account_name_type> lookup_witness_accounts(const std::string& lower_bound_name, uint32_t limit) const;

    /**
     * @brief Get account names in registration committee
     */
    std::set<account_name_type> lookup_registration_committee_members(const std::string& lower_bound_name,
                                                                      uint32_t limit) const;

    /**
     * @brief Get account names in development committee
     */
    std::set<account_name_type> lookup_development_committee_members(const std::string& lower_bound_name,
                                                                     uint32_t limit) const;

    /**
     * @brief Get proposals
     */
    std::vector<proposal_api_obj> lookup_proposals() const;

    /**
     * @brief Get development committee
     */
    development_committee_api_obj get_development_committee() const;

    /**
     * @brief Get registration committee
     */
    registration_committee_api_obj get_registration_committee() const;

    /**
     * @brief Get advertising property
     */
    advertising_property_api_obj get_advertising_property() const;

    /**
     * @brief Get the total number of witnesses registered with the blockchain
     */
    uint64_t get_witness_count() const;

    ////////////
    // Market //
    ////////////

    ////////////////////////////
    // Authority / validation //
    ////////////////////////////

    /// @brief Get a hexdump of the serialized binary form of a transaction
    std::string get_transaction_hex(const signed_transaction& trx) const;

    /**
     *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to
     * sign for
     *  and return the minimal subset of public keys that should add signatures to the transaction.
     */
    std::set<public_key_type> get_required_signatures(const signed_transaction& trx,
                                                      const flat_set<public_key_type>& available_keys) const;

    /**
     *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call
     * can
     *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref
     * get_required_signatures
     *  to get the minimum subset.
     */
    std::set<public_key_type> get_potential_signatures(const signed_transaction& trx) const;

    /**
     * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
     */
    bool verify_authority(const signed_transaction& trx) const;

    /*
     * @return true if the signers have enough authority to authorize an account
     */
    bool verify_account_authority(const std::string& name_or_id, const flat_set<public_key_type>& signers) const;

    /**
     *  if permlink is "" then it will return all votes for author
     */
    std::vector<vote_state> get_active_votes(const std::string& author, const std::string& permlink) const;
    std::vector<account_vote> get_account_votes(const std::string& voter) const;

    ////////////////////////////
    // NFT                    //
    ////////////////////////////

    nft_api_obj get_nft_by_id(nft_id_type id) const;
    nft_api_obj get_nft_by_name(const account_name_type& name) const;
    nft_api_obj get_nft_by_uuid(const uuid_type& uuid) const;
    std::vector<nft_api_obj> lookup_nft(nft_id_type id, uint32_t limit) const;

    game_round_api_obj get_game_round_by_uuid(const uuid_type& uuid) const;
    std::vector<game_round_api_obj> lookup_game_round(game_round_id_type id, uint32_t limit) const;

    ////////////////////////////
    // Handlers - not exposed //
    ////////////////////////////
    void on_api_startup();

private:
    std::shared_ptr<database_api_impl> my;
};
} // namespace app
} // namespace scorum

// clang-format off

FC_REFLECT( scorum::app::withdraw_route, (from_account)(to_account)(percent)(auto_vest) )

FC_REFLECT_ENUM( scorum::app::withdraw_route_type, (incoming)(outgoing)(all) )

FC_API(scorum::app::database_api,
   // Subscriptions
   (set_block_applied_callback)


   // Globals
   (get_config)
   (get_chain_id)
   (get_dynamic_global_properties)
   (get_witness_schedule)


   // Keys
   (get_key_references)

   // Accounts
   (get_accounts)
   (get_account_references)
   (lookup_account_names)
   (lookup_accounts)
   (get_account_count)
   (get_owner_history)
   (get_recovery_request)
   (get_escrow)
   (get_withdraw_routes)
   (get_account_bandwidth)
   (get_scorumpower_delegations)
   (get_expiring_scorumpower_delegations)

   // Authority / validation
   (get_transaction_hex)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)

   // votes
   (get_active_votes)
   (get_account_votes)

   // Witnesses
   (get_witnesses)
   (get_witness_by_account)
   (get_witnesses_by_vote)
   (lookup_witness_accounts)
   (get_witness_count)
   (get_active_witnesses)

    // Budget
   (get_budgets)
   (lookup_budget_owners)

   // Committee
   (lookup_registration_committee_members)
   (lookup_development_committee_members)
   (lookup_proposals)
   (get_registration_committee)
   (get_development_committee)
   (get_advertising_property)

    // Atomic Swap
   (get_atomicswap_contracts)
   (get_atomicswap_contract)

    // NFT
    (get_nft_by_id)
    (get_nft_by_name)
    (get_nft_by_uuid)
    (lookup_nft)
    (get_game_round_by_uuid)
    (lookup_game_round)
)

// clang-format on
