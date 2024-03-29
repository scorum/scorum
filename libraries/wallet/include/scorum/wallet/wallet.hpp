#pragma once

#include <fc/crypto/base58.hpp>
#include <fc/real128.hpp>
#include <functional>
#include <scorum/app/api.hpp>
#include <scorum/app/betting_api_objects.hpp>
#include <scorum/app/chain_api.hpp>
#include <scorum/app/scorum_api_objects.hpp>
#include <scorum/blockchain_history/api_objects.hpp>
#include <scorum/blockchain_history/schema/applied_operation.hpp>
#include <scorum/wallet/utils.hpp>

namespace scorum {
namespace wallet {

using namespace scorum::app;
using namespace scorum::chain;

using scorum::app::chain_capital_api_obj;
using scorum::blockchain_history::applied_operation;
using scorum::blockchain_history::applied_operation_type;
using scorum::blockchain_history::applied_withdraw_operation;
using scorum::blockchain_history::block_api_object;
using scorum::blockchain_history::signed_block_api_obj;

using transaction_handle_type = uint16_t;

struct memo_data
{
    static optional<memo_data> from_string(const std::string& str)
    {
        try
        {
            if (str.size() > sizeof(memo_data) && str[0] == '#')
            {
                auto data = fc::from_base58(str.substr(1));
                auto m = fc::raw::unpack<memo_data>(data);
                FC_ASSERT(std::string(m) == str);
                return m;
            }
        }
        catch (...)
        {
        }
        return optional<memo_data>();
    }

    public_key_type from;
    public_key_type to;
    uint64_t nonce = 0;
    uint32_t check = 0;
    std::vector<char> encrypted;

    operator std::string() const
    {
        auto data = fc::raw::pack(*this);
        auto base58 = fc::to_base58(data);
        return '#' + base58;
    }
};

struct wallet_data
{
    std::vector<char> cipher_keys; /** encrypted keys */

    std::string ws_server = "ws://localhost:8090";
    std::string ws_user;
    std::string ws_password;

    chain_id_type chain_id;
};

enum authority_type
{
    owner,
    active,
    posting
};

namespace detail {
class wallet_api_impl;
}

/**
 * @brief Wallet Management
 * @defgroup wallet_management Management
 * @ingroup wallet
 */

/**
 * @brief Wallet APIs
 * @defgroup wallet_api APIs
 * @ingroup wallet
 */

/**
 * @brief Wallet
 *
 * This wallet assumes it is connected to the database server with a
 * high-bandwidth, low-latency connection and performs minimal caching. This API
 * could be provided locally to be used by a web interface.
 *
 * @ingroup wallet
 */
class wallet_api
{
public:
    wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi);
    virtual ~wallet_api();

    using exit_func_type = std::function<void()>;

    void set_exit_func(exit_func_type);

    bool copy_wallet_file(const std::string& destination_filename);

    std::map<std::string, std::function<std::string(fc::variant, const fc::variants&)>> get_result_formatters() const;

    /**
     * Encrypt keys
     */
    void encrypt_keys();

    /**
     * Checks memos against private keys on account and imported in wallet
     */
    void check_memo(const std::string& memo, const account_api_obj& account) const;

    /**
     * Returns the encrypted memo if memo starts with '#' otherwise returns memo
     *
     * @ingroup wallet_main_api
     */
    std::string get_encrypted_memo(const std::string& from, const std::string& to, const std::string& memo);

    /**
     * Returns the decrypted memo if possible given wallet's known private keys
     *
     * @ingroup wallet_main_api
     */
    std::string decrypt_memo(const std::string& memo);

    /**
     * Close wallet application
     *
     * @ingroup wallet_management
     */
    void exit();

    /**
     * Returns info such as client version, git version of graphene/fc, version
     * of boost, openssl.
     *
     * @returns compile time info and client and dependencies versions
     *
     * @ingroup wallet_management
     */
    variant_object about() const;

    /**
     * Returns a list of all commands supported by the wallet API.
     *
     * This lists each command, along with its arguments and return types.
     * For more detailed help on a single command, use \c get_help()
     *
     * @returns a multi-line string suitable for displaying on a terminal
     *
     * @ingroup wallet_management
     */
    std::string help() const;

    /**
     * Returns detailed help on a single API command.
     *
     * @param method the name of the API command you want help with
     * @returns a multi-line string suitable for displaying on a terminal
     *
     * @ingroup wallet_management
     */
    std::string gethelp(const std::string& method) const;

    /**
     * Returns the current wallet filename.
     *
     * This is the filename that will be used when automatically saving the
     * wallet.
     *
     * @see set_wallet_filename()
     *
     * @return the wallet filename
     *
     * @ingroup wallet_management
     */
    std::string get_wallet_filename() const;

    /**
     * Checks whether the wallet has just been created and has not yet had a
     * password set.
     *
     * Calling \c set_password will transition the wallet to the locked state.
     *
     * @return true if the wallet is new
     *
     * @ingroup wallet_management
     */
    bool is_new() const;

    /**
     * Checks whether the wallet is locked (is unable to use its private keys).
     *
     * This state can be changed by calling \c lock() or \c unlock().
     *
     * @return true if the wallet is locked
     *
     * @ingroup wallet_management
     */
    bool is_locked() const;

    /**
     * Locks the wallet immediately.
     *
     * @ingroup wallet_management
     */
    void lock();

    /**
     * Unlocks the wallet.
     *
     * The wallet remain unlocked until the \c lock is called
     * or the program exits.
     *
     * @param password the password previously set with \c set_password()
     *
     * @ingroup wallet_management
     */
    void unlock(const std::string& password);

    /**
     * Sets a new password on the wallet.
     *
     * The wallet must be either 'new' or 'unlocked' to
     * execute this command.
     *
     * @ingroup wallet_management
     */
    void set_password(const std::string& password);

    /**
     * Loads a specified Graphene wallet.
     *
     * The current wallet is closed before the new wallet is loaded.
     *
     * @warning This does not change the filename that will be used for futrue
     * wallet writes, so this may cause you to overwrite your original wallet
     * unless you also call \c set_wallet_filename()
     *
     * @param wallet_filename the filename of the wallet JSON file to load. If
     *      \c wallet_filename is empty, it reloads the existing wallet file
     * @returns true if the specified wallet is loaded
     *
     * @ingroup wallet_management
     */
    bool load_wallet_file(const std::string& wallet_filename = "");

    /**
     * Saves the current wallet to the given filename.
     *
     * @warning This does not change the wallet filename that will be used for
     * futrue writes, so think of this function as 'Save a Copy As...' instead
     * of 'Save As...'.  Use \c set_wallet_filename() to make the filename
     * persist.
     *
     * @param wallet_filename the filename of the new wallet JSON file to create
     *      or overwrite. If \c wallet_filename is empty, save to the current
     *      filename.
     *
     * @ingroup wallet_management
     */
    void save_wallet_file(const std::string& wallet_filename = "");

    /**
     * Sets the wallet filename used for futrue writes.
     *
     * This does not trigger a save, it only changes the default filename that
     * will be used the next time a save is triggered.
     *
     * @param wallet_filename the new filename to use for futrue saves
     *
     * @ingroup wallet_management
     */
    void set_wallet_filename(const std::string& wallet_filename);

    /// @brief Wallet Helpers
    /// @defgroup wallet_help_api Helpers
    /// @ingroup wallet_api
    /// @{

    /**
     * Returns an uninitialized object representing a given blockchain operation.
     *
     * This returns a default-initialized object of the given type; it can be
     * used during early development of the wallet when we don't yet have custom
     * commands for creating all of the operations the blockchain supports.
     *
     * Any operation the blockchain supports can be created using the
     * transaction builder's \c add_operation_to_builder_transaction() , but to
     * do that from the CLI you need to know what the JSON form of the operation
     * looks like. This will give you a template you can fill in.  It's better
     * than nothing.
     *
     * @param operation_type the type of operation to return, must be one of the
     *      operations defined in `scorum/chain/operations.hpp`
     *      (e.g., "global_parameters_update_operation")
     * @return a default-constructed operation of the given type
     */
    operation get_prototype_operation(const std::string& operation_type);

    /**
     * Converts a signed_transaction in JSON form to its binary representation.
     *
     * @param tx the transaction to serialize
     * @returns the binary form of the transaction.  It will not be hex encoded,
     *          this returns a raw string that may have null characters embedded
     *          in it
     */
    std::string serialize_transaction(const signed_transaction& tx) const;

    /**
     * Signs a transaction.
     *
     * Given a fully-formed transaction that is only lacking signatrues, this
     * signs the transaction with the necessary keys and optionally broadcasts
     * the transaction
     *
     * @param tx the unsigned transaction
     * @param broadcast true if you wish to broadcast the transaction
     * @return the signed version of the transaction
     */
    annotated_signed_transaction sign_transaction(const signed_transaction& tx, bool broadcast = false);

    /**
     * Sets the amount of time in the futrue until a transaction expires.
     */
    void set_transaction_expiration(uint32_t seconds);

    /**
     * Network add nodes
     *
     * @param nodes
     */
    void network_add_nodes(const std::vector<std::string>& nodes);

    /**
     * Get connected peers
     *
     * @return std::vector<variant>
     */
    std::vector<variant> network_get_connected_peers();

    /// @}

    /// @brief Wallet Key API
    /// @defgroup wallet_key_api Key
    /// @ingroup wallet_api
    /// @{

    /**
     * Imports a WIF Private Key into the wallet to be used to sign transactions
     * by an account.
     *
     * example: import_key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
     *
     * @param wif_key the WIF Private Key to import
     */
    bool import_key(const std::string& wif_key);

    /**
     * Suggests a safe brain key to use for creating your account.
     *
     * \c create_account_with_brain_key() requires you to specify a 'brain key',
     * a long passphrase that provides enough entropy to generate cyrptographic
     * keys.  This function will suggest a suitably random string that should
     * be easy to write down (and, with effort, memorize).
     *
     * @returns a suggested brain_key
     */
    brain_key_info suggest_brain_key() const;

    /**
     * Dumps all private keys owned by the wallet.
     *
     * The keys are printed in WIF format.  You can import these keys into
     * another wallet using \c import_key()
     *
     * @returns a map containing the private keys, indexed by their public key
     */
    std::map<public_key_type, std::string> list_keys();

    /**
     * Get the WIF private key corresponding to a public key.  The
     * private key must already be in the wallet.
     */
    std::string get_private_key(const public_key_type& pubkey) const;

    /**
     * Get private key from password
     *
     * @param account
     * @param role active | owner | posting | memo
     * @param password
     */
    std::pair<public_key_type, std::string> get_private_key_from_password(const std::string& account,
                                                                          const std::string& role,
                                                                          const std::string& password) const;

    /**
     * Transforms a brain key to reduce the chance of errors when re-entering
     * the key from memory.
     *
     * This takes a user-supplied brain key and normalizes it into the form used
     * for generating private keys.  In particular, this upper-cases all ASCII
     * characters and collapses multiple spaces into one.
     *
     * @param s the brain key as supplied by the user
     * @returns the brain key in its normalized form
     */
    std::string normalize_brain_key(const std::string& s) const;

    /// @}

    /// @brief Wallet Blockchain history API
    /// @defgroup wallet_history_api Blockchain history
    /// @ingroup wallet_api
    /// @{

    /**
     * Returns the information about a block header
     *
     * @param num Block num
     * @returns Header block data on the blockchain
     */
    optional<block_header> get_block_header(uint32_t num) const;

    /**
     * Returns the information about a block
     *
     * @param num Block num
     * @returns Public block data on the blockchain
     */
    optional<signed_block_api_obj> get_block(uint32_t num) const;

    /**
     * Returns information about the block headers in range [from-limit, from]
     *
     * @param num Block num, -1 means most recent, limit is the number of blocks
     *      before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     * @return map of block headers
     */
    std::map<uint32_t, block_header> get_block_headers_history(uint32_t num, uint32_t limit) const;

    /**
     * Returns information about the blocks in range [from-limit, from]
     *
     * @param num Block num, -1 means most recent, limit is the number of blocks
     *      before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     * @return map of blocks
     */
    std::map<uint32_t, signed_block_api_obj> get_blocks_history(uint32_t num, uint32_t limit) const;

    /**
     * Returns sequence of operations included/generated in a specified block
     *
     * @param block_num Block height of specified block
     * @param type_of_operation Operations type (all = 0, not_virt = 1, virt =
     *      2, market = 3)
     * @return map of applied operations
     */
    std::map<uint32_t, applied_operation> get_ops_in_block(uint32_t block_num,
                                                           applied_operation_type type_of_operation) const;

    /**
     * This method returns all operations in ids range [from-limit, from]
     *
     * @param from_op the operation number, -1 means most recent, limit is the
     *      number of operations before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     * @param type_of_operation Operations type (all = 0, not_virt = 1, virt =
     *      2, market = 3)
     * @return map of applied operations
     */
    std::map<uint32_t, applied_operation>
    get_ops_history(uint32_t from_op, uint32_t limit, applied_operation_type type_of_operation) const;

    /**
     * This method returns all operations in timestamp range [from, to] by pages
     * [from_op-limit, from_op]
     *
     * @param from the time from start searching operations
     * @param to the time until end searching operations
     * @param from_op the operation number, -1 means most recent, limit is the
     *      number of operations before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     * @return map of applied operations
     */
    std::map<uint32_t, applied_operation> get_ops_history_by_time(const fc::time_point_sec& from,
                                                                  const fc::time_point_sec& to,
                                                                  uint32_t from_op,
                                                                  uint32_t limit) const;

    /**
     * Retrieve the list of blocks from block log in range [from-limit, from]
     *
     * @param from Height of the block to be returned
     * @param limit the maximum number of blocks that can be queried (0 to 100],
     *      must be less than from
     * @return the list of signed blocks
     */
    std::vector<block_api_object> get_blocks(uint32_t from, uint32_t limit) const;

    /// @}

    /// @brief Wallet Account/Market/Blog API
    /// @defgroup wallet_main_api Account/Transfers/Posts
    /// @ingroup wallet_api
    /// @{

    /**
     * Returns info about the current state of the blockchain
     */
    variant info();

    /**
     * Gets all money circulating between funds and users.
     */
    chain_capital_api_obj get_chain_capital() const;

    /**
     * Returns the list of witnesses producing blocks in the current round (21
     * Blocks)
     */
    std::vector<account_name_type> get_active_witnesses() const;

    /**
     * Gets the account information for all accounts for which this wallet has a
     * private key
     */
    std::vector<account_api_obj> list_my_accounts();

    /**
     * Lists all accounts registered in the blockchain.
     *
     * This returns a list of all account names and their account ids, sorted by
     * account name.
     *
     * Use the \c lowerbound and \c limit parameters to page through the list.
     * To retrieve all accounts, start by setting \c lowerbound to the empty
     * string \c "", and then each iteration, pass the last account name
     * returned as the \c lowerbound for the next \c list_accounts() call.
     *
     * @param lowerbound the name of the first account to return.  If the named
     *      account does not exist, the list will start at the account that
     *      comes after \c lowerbound
     * @param limit the maximum number of accounts to return (max: 1000)
     * @returns a list of accounts mapping account names to account ids
     */
    std::set<std::string> list_accounts(const std::string& lowerbound, uint32_t limit);

    /**
     * Returns information about the given account.
     *
     * @param account_name the name of the account to provide information about
     * @returns the public account data stored in the blockchain
     */
    account_api_obj get_account(const std::string& account_name) const;

    /**
     * Returns balance information about the given account.
     *
     * @param account_name the name of the account to provide information about
     * @returns the public account data stored in the blockchain
     */
    account_balance_info_api_obj get_account_balance(const std::string& account_name) const;

    /**
     * Create account
     *
     * This method will genrate new owner, active, and memo keys for the new
     * account which will be controlable by this wallet. There is a fee
     * associated with account creation that is paid by the creator. The current
     * account creation fee can be found with the 'info' wallet command.
     *
     * @param creator The account creating the new account
     * @param newname The name of the new account
     * @param json_meta JSON Metadata associated with the new account
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_account(const std::string& creator,
                                                const std::string& newname,
                                                const std::string& json_meta,
                                                bool broadcast);

    /**
     * Create account with keys
     *
     * This method is used by faucets to create new accounts for other users
     * which must provide their desired keys. The resulting account may not be
     * controllable by this wallet. There is a fee associated with account
     * creation that is paid by the creator. The current account creation fee
     * can be found with the 'info' wallet command.
     *
     * @param creator The account creating the new account
     * @param newname The name of the new account
     * @param json_meta JSON Metadata associated with the new account
     * @param owner public owner key of the new account
     * @param active public active key of the new account
     * @param posting public posting key of the new account
     * @param memo public memo key of the new account
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_account_with_keys(const std::string& creator,
                                                          const std::string& newname,
                                                          const std::string& json_meta,
                                                          const public_key_type& owner,
                                                          const public_key_type& active,
                                                          const public_key_type& posting,
                                                          const public_key_type& memo,
                                                          bool broadcast) const;

    /**
     * Create account delegated
     *
     * This method will genrate new owner, active, and memo keys for the new
     * account which will be controlable by this wallet. There is a fee
     * associated with account creation that is paid by the creator. The current
     * account creation fee can be found with the 'info' wallet command.
     *
     * These accounts are created with combination of SCR and delegated SP
     *
     * @param creator The account creating the new account
     * @param scorum_fee The amount of the fee to be paid with SCR
     * @param delegated_scorumpower The amount of the fee to be paid with
     *      delegation
     * @param new_account_name The name of the new account
     * @param json_meta JSON Metadata associated with the new account
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_account_delegated(const std::string& creator,
                                                          const asset& scorum_fee,
                                                          const asset& delegated_scorumpower,
                                                          const std::string& new_account_name,
                                                          const std::string& json_meta,
                                                          bool broadcast);

    /**
     * Create account with keys delegated
     *
     * This method is used by faucets to create new accounts for other users
     * which must provide their desired keys. The resulting account may not be
     * controllable by this wallet. There is a fee associated with account
     * creation that is paid by the creator. The current account creation fee
     * can be found with the 'info' wallet command.
     *
     * These accounts are created with combination of SCR and delegated SP
     *
     * @param creator The account creating the new account
     * @param scorum_fee The amount of the fee to be paid with SCR
     * @param delegated_scorumpower The amount of the fee to be paid with
     *      delegation
     * @param newname The name of the new account
     * @param json_meta JSON Metadata associated with the new account
     * @param owner public owner key of the new account
     * @param active public active key of the new account
     * @param posting public posting key of the new account
     * @param memo public memo key of the new account
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_account_with_keys_delegated(const std::string& creator,
                                                                    const asset& scorum_fee,
                                                                    const asset& delegated_scorumpower,
                                                                    const std::string& newname,
                                                                    const std::string& json_meta,
                                                                    const public_key_type& owner,
                                                                    const public_key_type& active,
                                                                    const public_key_type& posting,
                                                                    const public_key_type& memo,
                                                                    bool broadcast) const;
    /**
     * This method will generate new owner, active, and memo keys for the new
     * account which will be controllable by this wallet. There is a fee
     * associated with account creation that is paid by the creator. The current
     * account creation fee can be found with the 'info' wallet command.
     *
     * @param creator The account creating the new account
     * @param newname The name of the new account
     * @param json_meta JSON Metadata associated with the new account
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_account_by_committee(const std::string& creator,
                                                             const std::string& newname,
                                                             const std::string& json_meta,
                                                             bool broadcast);

    /**
     * This method is used by faucets to create new accounts for other users
     * which must provide their desired keys. The resulting account accepts
     * bonus from registration pool. Creator must belong registration committee.
     *
     * @param creator The committee member creating the new account
     * @param newname The name of the new account
     * @param json_meta JSON Metadata associated with the new account
     * @param owner public owner key of the new account
     * @param active public active key of the new account
     * @param posting public posting key of the new account
     * @param memo public memo key of the new account
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_account_by_committee_with_keys(const std::string& creator,
                                                                       const std::string& newname,
                                                                       const std::string& json_meta,
                                                                       const public_key_type& owner,
                                                                       const public_key_type& active,
                                                                       const public_key_type& posting,
                                                                       const public_key_type& memo,
                                                                       bool broadcast) const;
    /**
     * This method updates the keys of an existing account.
     *
     * @param accountname The name of the account
     * @param json_meta New JSON Metadata to be associated with the account
     * @param owner New public owner key for the account
     * @param active New public active key for the account
     * @param posting New public posting key for the account
     * @param memo New public memo key for the account
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_account(const std::string& accountname,
                                                const std::string& json_meta,
                                                const public_key_type& owner,
                                                const public_key_type& active,
                                                const public_key_type& posting,
                                                const public_key_type& memo,
                                                bool broadcast) const;

    /**
     * This method updates the key of an authority for an existing account.
     *
     * @warning You can create impossible authorities using this method. The
     * method will fail if you create an impossible owner authority, but will
     * allow impossible active and posting authorities.
     *
     * @param account_name The name of the account whose authority you wish to
     *      update
     * @param type The authority type. e.g. owner, active, or posting
     * @param key The public key to add to the authority
     * @param weight The weight the key should have in the authority. A weight
     *      of 0 indicates the removal of the key.
     * @param broadcast true if you wish to broadcast the transaction.
     */
    annotated_signed_transaction update_account_auth_key(const std::string& account_name,
                                                         const authority_type& type,
                                                         const public_key_type& key,
                                                         authority_weight_type weight,
                                                         bool broadcast);

    /**
     * This method updates the account of an authority for an existing account.
     *
     * @warning You can create impossible authorities using this method. The
     * method will fail if you create an impossible owner authority, but will
     * allow impossible active and posting authorities.
     *
     * @param account_name The name of the account whose authority you wish to
     *      update
     * @param type The authority type. e.g. owner, active, or posting
     * @param auth_account The account to add the the authority
     * @param weight The weight the account should have in the authority. A
     *      weight of 0 indicates the removal of the account.
     * @param broadcast true if you wish to broadcast the transaction.
     */
    annotated_signed_transaction update_account_auth_account(const std::string& account_name,
                                                             authority_type type,
                                                             const std::string& auth_account,
                                                             authority_weight_type weight,
                                                             bool broadcast);

    /**
     * This method updates the weight threshold of an authority for an account.
     *
     * @warning You can create impossible authorities using this method as well
     * as implicitly met authorities. The method will fail if you create an
     * implicitly true authority and if you create an impossible owner
     * authoroty, but will allow impossible active and posting authorities.
     *
     * @param account_name The name of the account whose authority you wish to
     *      update
     * @param type The authority type. e.g. owner, active, or posting
     * @param threshold The weight threshold required for the authority to be
     *      met
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_account_auth_threshold(const std::string& account_name,
                                                               authority_type type,
                                                               uint32_t threshold,
                                                               bool broadcast);

    /**
     * This method updates the account JSON metadata
     *
     * @param account_name The name of the account you wish to update
     * @param json_meta The new JSON metadata for the account. This overrides
     *      existing metadata
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    update_account_meta(const std::string& account_name, const std::string& json_meta, bool broadcast);

    /**
     * This method updates the memo key of an account
     *
     * @param account_name The name of the account you wish to update
     * @param key The new memo public key
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    update_account_memo_key(const std::string& account_name, const public_key_type& key, bool broadcast);

    /**
     * This method delegates SP from one account to another.
     *
     * @param delegator The name of the account delegating SP
     * @param delegatee The name of the account receiving SP
     * @param scorumpower The amount of SP to delegate
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction delegate_scorumpower(const std::string& delegator,
                                                      const std::string& delegatee,
                                                      const asset& scorumpower,
                                                      bool broadcast);

    /**
     * This method delegates SP from registration pool to account.
     *
     * @param reg_committee_member Registration committee member
     * @param delegatee The name of the account receiving SP
     * @param scorumpower The amount of SP to delegate
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction delegate_scorumpower_from_reg_pool(const std::string& reg_committee_member,
                                                                    const std::string& delegatee,
                                                                    const asset& scorumpower,
                                                                    bool broadcast);

    /**
     * Lists all witnesses registered in the blockchain.
     *
     * This returns a list of all account names that own witnesses, and the
     * associated witness id, sorted by name. This lists witnesses whether they
     * are currently voted in or not.
     *
     * Use the \c lowerbound and limit parameters to page through the list. To
     * retrieve all witnesss, start by setting \c lowerbound to the empty string
     * \c "", and then each iteration, pass the last witness name returned as
     * the \c lowerbound for the next \c list_witnesss() call.
     *
     * @param lowerbound the name of the first witness to return. If the named
     *      witness does not exist, the list will start at the witness that
     *      comes after \c lowerbound
     * @param limit the maximum number of witnesss to return (max: 1000)
     * @returns a list of witnesss mapping witness names to witness ids
     */
    std::set<account_name_type> list_witnesses(const std::string& lowerbound, uint32_t limit);

    /**
     * Returns information about the given witness.
     *
     * @param owner_account the name or id of the witness account owner, or the
     *      id of the witness
     * @returns the information about the witness stored in the block chain
     */
    optional<witness_api_obj> get_witness(const std::string& owner_account);

    /**
     * Update a witness object owned by the given account.
     *
     * @param witness_name The name of the witness account.
     * @param url A URL containing some information about the witness.
     *      The empty string makes it remain the same.
     * @param block_signing_key The new block signing public key.
     *      The empty string disables block production.
     * @param props The chain properties the witness is voting on.
     * @param broadcast true if you wish to broadcast the transaction.
     */
    annotated_signed_transaction update_witness(const std::string& witness_name,
                                                const std::string& url,
                                                const public_key_type& block_signing_key,
                                                const chain_properties& props,
                                                bool broadcast = false);

    /**
     * Set the voting proxy for an account.
     *
     * If a user does not wish to take an active part in voting, they can choose
     * to allow another account to vote their stake.
     *
     * Setting a vote proxy does not remove your previous votes from the
     * blockchain, they remain there but are ignored.  If you later null out
     * your vote proxy, your previous votes will take effect again.
     *
     * This setting can be changed at any time.
     *
     * @param account_to_modify the name or id of the account to update
     * @param proxy the name of account that should proxy to, or empty string to
     *       have no proxy
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    set_voting_proxy(const std::string& account_to_modify, const std::string& proxy, bool broadcast = false);

    /**
     * Vote for a witness to become a block producer.
     *
     * By default an account has not voted positively or negatively for a
     * witness. The account can either vote for with positively votes or against
     * with negative votes. The vote will remain until updated with another
     * vote. Vote strength is determined by the accounts scorumpower.
     *
     * @param account_to_vote_with The account voting for a witness
     * @param witness_to_vote_for The witness that is being voted for
     * @param approve true if the account is voting for the account to be able
     *       to be a block produce
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction vote_for_witness(const std::string& account_to_vote_with,
                                                  const std::string& witness_to_vote_for,
                                                  bool approve = true,
                                                  bool broadcast = false);

    /**
     * Transfer funds from one account to another.
     *
     * @param from The account the funds are coming from
     * @param to The account the funds are going to
     * @param amount The funds being transferred. i.e. "100.000000000 SCR"
     * @param memo A memo for the transactionm, encrypted with the to account's
     *      public memo key
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction transfer(const std::string& from,
                                          const std::string& to,
                                          const asset& amount,
                                          const std::string& memo,
                                          bool broadcast = false);

    /**
     * Transfer funds from one account to another using escrow.
     *
     * @param from The account the funds are coming from
     * @param to The account the funds are going to
     * @param agent The account acting as the agent in case of dispute
     * @param escrow_id A unique id for the escrow transfer. (from, escrow_id)
     *      must be a unique pair
     * @param scorum_amount The amount of SCR to transfer
     * @param fee The fee paid to the agent
     * @param ratification_deadline The deadline for 'to' and 'agent' to approve
     *      the escrow transfer
     * @param escrow_expiration The expiration of the escrow transfer, after
     *      which either party can claim the funds
     * @param json_meta JSON encoded meta data
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction escrow_transfer(const std::string& from,
                                                 const std::string& to,
                                                 const std::string& agent,
                                                 uint32_t escrow_id,
                                                 const asset& scorum_amount,
                                                 const asset& fee,
                                                 time_point_sec ratification_deadline,
                                                 time_point_sec escrow_expiration,
                                                 const std::string& json_meta,
                                                 bool broadcast = false);

    /**
     * Approve a proposed escrow transfer. Funds cannot be released until after
     * approval. This is in lieu of requiring multi-sig on escrow_transfer
     *
     * @param from The account that funded the escrow
     * @param to The destination of the escrow
     * @param agent The account acting as the agent in case of dispute
     * @param who The account approving the escrow transfer (either 'to' or
     *      'agent')
     * @param escrow_id A unique id for the escrow transfer
     * @param approve true to approve the escrow transfer, otherwise cancels it
     *      and refunds 'from'
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction escrow_approve(const std::string& from,
                                                const std::string& to,
                                                const std::string& agent,
                                                const std::string& who,
                                                uint32_t escrow_id,
                                                bool approve,
                                                bool broadcast = false);

    /**
     * Raise a dispute on the escrow transfer before it expires
     *
     * @param from The account that funded the escrow
     * @param to The destination of the escrow
     * @param agent The account acting as the agent in case of dispute
     * @param who The account raising the dispute (either 'from' or 'to')
     * @param escrow_id A unique id for the escrow transfer
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction escrow_dispute(const std::string& from,
                                                const std::string& to,
                                                const std::string& agent,
                                                const std::string& who,
                                                uint32_t escrow_id,
                                                bool broadcast = false);

    /**
     * Release funds help in escrow
     *
     * @param from The account that funded the escrow
     * @param to The account the funds are originally going to
     * @param agent The account acting as the agent in case of dispute
     * @param who The account authorizing the release
     * @param receiver The account that will receive funds being released
     * @param escrow_id A unique id for the escrow transfer
     * @param scorum_amount The amount of SCR that will be released
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction escrow_release(const std::string& from,
                                                const std::string& to,
                                                const std::string& agent,
                                                const std::string& who,
                                                const std::string& receiver,
                                                uint32_t escrow_id,
                                                const asset& scorum_amount,
                                                bool broadcast = false);

    /**
     * Transfer SCR into a scorumpower fund represented by scorumpower (SP).
     *
     * SP are required to vesting for a minimum of one coin year and can be
     * withdrawn once a week over a two year withdraw period. SP are protected
     * against dilution up until 90% of SCR is vesting.
     *
     * @param from The account the SCR is coming from
     * @param to The account getting the SP
     * @param amount The amount of SCR to scorum power i.e. "100.000000000 SCR"
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction transfer_to_scorumpower(const std::string& from,
                                                         const std::string& to,
                                                         const asset& amount,
                                                         bool broadcast = false);

    /**
     * Set up a vesting withdraw request.
     *
     * The request is fulfilled once a week over the next 13 weeks.
     *
     * @param from The account the SP are withdrawn from
     * @param scorumpower The amount of SP to withdraw over the next 13 weeks.
     *      Each week (amount/13) shares are withdrawn and deposited back as SCR
     *      i.e. "10.000000000 SP"
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    withdraw_scorumpower(const std::string& from, const asset& scorumpower, bool broadcast = false);

    /**
     * Set up a vesting withdraw route.
     *
     * When scorumpower are withdrawn, they will be routed to these accounts
     * based on the specified weights.
     *
     * @param from The account the SP are withdrawn from.
     * @param to   The account receiving either SP or SCR.
     * @param percent The percent of the withdraw to go to the 'to' account.
     *      This is denoted in hundredths of a percent.
     *      i.e. 100 is 1% and 10000 is 100%.
     *      This value must be between 1 and 100000
     * @param auto_vest Set to true if the 'to' account should receive the SP as
     *      SP, or false if it should receive them as SCR.
     * @param broadcast true if you wish to broadcast the transaction.
     */
    annotated_signed_transaction set_withdraw_scorumpower_route(
        const std::string& from, const std::string& to, uint16_t percent, bool auto_vest, bool broadcast = false);

    /**
     * Returns vesting withdraw routes for an account.
     *
     * @param account Account to query routes
     * @param type Withdraw type type [incoming, outgoing, all]
     */
    std::vector<withdraw_route> get_withdraw_routes(const std::string& account, withdraw_route_type type = all) const;

    /**
     * Returns transaction by ID.
     *
     * @param trx_id
     * @return annotated_signed_transaction
     */
    annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;


    /**
     * Post or update a comment.
     *
     * @param author the name of the account authoring the comment
     * @param permlink the accountwide unique permlink for the comment
     * @param parent_author can be null if this is a top level comment
     * @param parent_permlink becomes category if parent_author is ""
     * @param title the title of the comment
     * @param body the body of the comment
     * @param json the json metadata of the comment
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction post_comment(const std::string& author,
                                              const std::string& permlink,
                                              const std::string& parent_author,
                                              const std::string& parent_permlink,
                                              const std::string& title,
                                              const std::string& body,
                                              const std::string& json,
                                              bool broadcast);

    /**
     * Vote on a comment to be paid SCR
     *
     * @param voter The account voting
     * @param author The author of the comment to be voted on
     * @param permlink The permlink of the comment to be voted on.
     *      (author, permlink) is a unique pair
     * @param weight The weight [-10000,10000] of the vote
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction vote(const std::string& voter,
                                      const std::string& author,
                                      const std::string& permlink,
                                      int16_t weight,
                                      bool broadcast);

    /**
     * Challenge a user's authority. The challenger pays a fee to the challenged
     * which is depositted as Scorum Power. Until the challenged proves their
     * active key, all posting rights are revoked.
     *
     * @param challenger The account issuing the challenge
     * @param challenged The account being challenged
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    challenge(const std::string& challenger, const std::string& challenged, bool broadcast);

    /**
     * Create an account recovery request as a recover account. The syntax for
     * this command contains a serialized authority object so there is an
     * example below on how to pass in the authority.
     *
     * request_account_recovery "your_account" "account_to_recover"
     * {"weight_threshold": 1,"account_auths": [], "key_auths":
     * [["new_public_key",1]]} true
     *
     * @param recovery_account The name of your account
     * @param account_to_recover The name of the account you are trying to
     *      recover
     * @param new_authority The new owner authority for the recovered account.
     *      This should be given to you by the holder of the compromised or lost
     *      account.
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction request_account_recovery(const std::string& recovery_account,
                                                          const std::string& account_to_recover,
                                                          const authority& new_authority,
                                                          bool broadcast);

    /**
     * Recover your account using a recovery request created by your recovery
     * account.
     *
     * The syntax for this commain contains a serialized authority object,
     * so there is an example below on how to pass in the authority.
     *
     * recover_account "your_account" {"weight_threshold": 1,"account_auths":
     * [], "key_auths": [["old_public_key",1]]}
     * {"weight_threshold": 1,"account_auths": [], "key_auths":
     * [["new_public_key",1]]} true
     *
     * @param account_to_recover The name of your account
     * @param recent_authority A recent owner authority on your account
     * @param new_authority The new authority that your recovery account used in
     *      the account recover request.
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction recover_account(const std::string& account_to_recover,
                                                 const authority& recent_authority,
                                                 const authority& new_authority,
                                                 bool broadcast);

    /**
     * Change your recovery account after a 30 day delay.
     *
     * @param owner The name of your account
     * @param new_recovery_account The name of the recovery account you wish to
     *      have
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    change_recovery_account(const std::string& owner, const std::string& new_recovery_account, bool broadcast);

    /**
     * Prove an account's active authority, fulfilling a challenge, restoring
     * posting rights, and making the account immune to challenge for 24 hours.
     *
     * @param challenged The account that was challenged and is proving its
     *      authority.
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction prove(const std::string& challenged, bool broadcast);

    /**
     * @brief Get the owner history object
     *
     * @param account
     * @return std::vector<owner_authority_history_api_obj>
     */
    std::vector<owner_authority_history_api_obj> get_owner_history(const std::string& account) const;

    /**
     * Account operations have sequence numbers from 0 to N where N is the most
     * recent operation.
     *
     * This method returns operations in the range [from-limit, from]
     *
     * @param account account whose history will be returned
     * @param from the absolute sequence number, -1 means most recent, limit is
     *      the number of operations before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     */
    std::map<uint32_t, applied_operation>
    get_account_history(const std::string& account, uint64_t from, uint32_t limit);

    /**
     * Account operations have sequence numbers from 0 to N where N is the most
     * recent operation.
     *
     * This method returns operations in the range [from-limit, from]
     *
     * @param account account whose history will be returned
     * @param from the absolute sequence number, -1 means most recent, limit is
     *      the number of operations before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     */
    std::map<uint32_t, applied_operation>
    get_account_scr_to_scr_transfers(const std::string& account, uint64_t from, uint32_t limit);

    /**
     * Account operations have sequence numbers from 0 to N where N is the most
     * recent operation.
     *
     * This method returns operations in the range [from-limit, from]
     *
     * @param account account whose history will be returned
     * @param from the absolute sequence number, -1 means most recent, limit is
     *      the number of operations before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     */
    std::map<uint32_t, applied_operation>
    get_account_scr_to_sp_transfers(const std::string& account, uint64_t from, uint32_t limit);

    /**
     * Account operations have sequence numbers from 0 to N where N is the most
     * recent operation.
     *
     * This method returns operations in the range [from-limit, from]
     *
     * @param account account whose history will be returned
     * @param from the absolute sequence number, -1 means most recent, limit is
     *      the number of operations before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     */
    std::map<uint32_t, applied_withdraw_operation>
    get_account_sp_to_scr_transfers(const std::string& account, uint64_t from, uint32_t limit);

    /**
     * Decline voting rights
     *
     * @param account
     * @param decline
     * @param broadcast
     */
    annotated_signed_transaction decline_voting_rights(const std::string& account, bool decline, bool broadcast);

    /// @}

    /// @brief Wallet Advertising API
    /// @defgroup wallet_advertising_api Advertising
    /// @ingroup wallet_api
    /// @{

    /**
     * Gets the budget information for all my budgets (list_my_accounts)
     *
     * @return std::vector<budget_api_obj>
     */
    std::vector<budget_api_obj> list_my_budgets();

    /**
     * Gets the list of all budget (for POST type) owners (look list_accounts to
     * understand input parameters)
     *
     * @param lowerbound
     * @param limit
     * @return std::set<std::string>
     */
    std::set<std::string> list_post_budget_owners(const std::string& lowerbound, uint32_t limit);

    /**
     * Gets the list of all budget (for BANNER type) owners (look list_accounts
     * to understand input parameters)
     *
     * @param lowerbound
     * @param limit
     * @return std::set<std::string>
     */
    std::set<std::string> list_banner_budget_owners(const std::string& lowerbound, uint32_t limit);

    /**
     * Gets the budget (for POST type) information for certain account
     *
     * @param account_name
     * @return std::vector<budget_api_obj>
     */
    std::vector<budget_api_obj> get_post_budgets(const std::string& account_name);

    /**
     * Gets the budget (for BANNER type) information for certain account
     *
     * @param account_name
     * @return std::vector<budget_api_obj>
     */
    std::vector<budget_api_obj> get_banner_budgets(const std::string& account_name);

    /**
     * This method will create new budget (for POST type) linked to owner
     * account.
     *
     * @warning The owner account must have sufficient balance for budget
     *
     * @param owner the futrue owner of creating budget
     * @param uuid budget uuid
     * @param json_metadata the budget target identity (post or other)
     * @param balance
     * @param start the time to start allocation cash from  budget
     * @param deadline the deadline time to close budget (even if there is rest
     *      of balance)
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_budget_for_post(const std::string& owner,
                                                        const uuid_type& uuid,
                                                        const std::string& json_metadata,
                                                        const asset& balance,
                                                        const time_point_sec& start,
                                                        const time_point_sec& deadline,
                                                        bool broadcast);

    /**
     * This method will create new budget (for BANNER type) linked to owner
     * account.
     *
     * @warning The owner account must have sufficient balance for budget
     *
     * @param owner the futrue owner of creating budget
     * @param uuid budget uuid
     * @param json_metadata the budget target identity (post or other)
     * @param balance
     * @param start the time to start allocation cash from  budget
     * @param deadline the deadline time to close budget (even if there is rest
     *      of balance)
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_budget_for_banner(const std::string& owner,
                                                          const uuid_type& uuid,
                                                          const std::string& json_metadata,
                                                          const asset& balance,
                                                          const time_point_sec& start,
                                                          const time_point_sec& deadline,
                                                          bool broadcast);

    /**
     * This method will update budget metadata for banner by owner name and
     * budget uuid
     *
     * @param owner account name
     * @param uuid budget uuid
     * @param json_metadata some metadata
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_budget_for_banner(const std::string& owner,
                                                          const uuid_type& uuid,
                                                          const std::string& json_metadata,
                                                          bool broadcast);

    /**
     * This method will update budget metadata for post by owner name and budget
     * uuid
     *
     * @param owner account name
     * @param uuid budget uuid
     * @param json_metadata some metadata
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_budget_for_post(const std::string& owner,
                                                        const uuid_type& uuid,
                                                        const std::string& json_metadata,
                                                        bool broadcast);

    /**
     * Closing the budget (for POST type). The budget rest is returned to the
     * owner's account
     *
     * @param uuid
     * @param owner
     * @param broadcast
     */
    annotated_signed_transaction close_budget_for_post(const uuid_type& uuid, const std::string& owner, bool broadcast);

    /**
     * Closing the budget (for BANNER type). The budget rest is returned to the
     * owner's account
     *
     * @param uuid
     * @param owner
     * @param broadcast
     */
    annotated_signed_transaction
    close_budget_for_banner(const uuid_type& uuid, const std::string& owner, bool broadcast);

    /**
     * Closing the budget (for POST type). The budget rest is returned to the
     * owner's account
     *
     * @param uuid
     * @param moderator
     * @param broadcast
     */
    annotated_signed_transaction
    close_budget_for_post_by_moderator(const uuid_type& uuid, const std::string& moderator, bool broadcast);

    /**
     * Closing the budget (for BANNER type). The budget rest is returned to the
     * owner's account
     *
     * @param uuid
     * @param moderator
     * @param broadcast
     */
    annotated_signed_transaction
    close_budget_for_banner_by_moderator(const uuid_type& uuid, const std::string& moderator, bool broadcast);

    /// @}

    /// @brief Wallet Committee API
    /// @defgroup wallet_committee_api Committee
    /// @ingroup wallet_api
    /// @{

    /**
     * Vote for committee proposal
     *
     * @param account_to_vote_with
     * @param proposal_id
     * @param broadcast
     */
    annotated_signed_transaction
    vote_for_committee_proposal(const std::string& account_to_vote_with, int64_t proposal_id, bool broadcast);

    /**
     * Create proposal for inviting new member in to the registration commmittee
     *
     * @param inviter
     * @param invitee
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction registration_committee_add_member(const std::string& inviter,
                                                                   const std::string& invitee,
                                                                   uint32_t lifetime_sec,
                                                                   bool broadcast);

    /**
     * Create proposal for excluding member from registration committee
     *
     * @param initiator
     * @param dropout
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction registration_committee_exclude_member(const std::string& initiator,
                                                                       const std::string& dropout,
                                                                       uint32_t lifetime_sec,
                                                                       bool broadcast);

    /**
     * List registration committee members
     *
     * @param lowerbound
     * @param limit
     * @return std::set<account_name_type>
     */
    std::set<account_name_type> list_registration_committee(const std::string& lowerbound, uint32_t limit);

    /**
     * Get the registration committee object
     *
     * @return registration_committee_api_obj
     */
    registration_committee_api_obj get_registration_committee();

    /**
     * List proposals
     *
     * @return std::vector<proposal_api_obj>
     */
    std::vector<proposal_api_obj> list_proposals();

    /**
     * Change registration committee quorum for adding new member
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     * @return annotated_signed_transaction
     */
    annotated_signed_transaction registration_committee_change_add_member_quorum(const std::string& creator,
                                                                                 uint64_t quorum_percent,
                                                                                 uint32_t lifetime_sec,
                                                                                 bool broadcast);

    /**
     * Change registration committee quorum for excluding member
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     * @return annotated_signed_transaction
     */
    annotated_signed_transaction registration_committee_change_exclude_member_quorum(const std::string& creator,
                                                                                     uint64_t quorum_percent,
                                                                                     uint32_t lifetime_sec,
                                                                                     bool broadcast);

    /**
     * Change registration committee for changing add/exclude quorum
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction registration_committee_change_base_quorum(const std::string& creator,
                                                                           uint64_t quorum_percent,
                                                                           uint32_t lifetime_sec,
                                                                           bool broadcast);

    /**
     * Create proposal for inviting new member in to the development commmittee
     *
     * @param initiator
     * @param invitee
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_add_member(const std::string& initiator,
                                                                  const std::string& invitee,
                                                                  uint32_t lifetime_sec,
                                                                  bool broadcast);

    /**
     * Create proposal for excluding member from development committee
     *
     * @param initiator
     * @param dropout
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_exclude_member(const std::string& initiator,
                                                                      const std::string& dropout,
                                                                      uint32_t lifetime_sec,
                                                                      bool broadcast);

    /**
     * List development committee members
     *
     * @param lowerbound
     * @param limit
     * @return std::set<account_name_type>
     */
    std::set<account_name_type> list_development_committee(const std::string& lowerbound, uint32_t limit);

    /**
     * Change development committee quorum for adding new member
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_change_add_member_quorum(const std::string& creator,
                                                                                uint64_t quorum_percent,
                                                                                uint32_t lifetime_sec,
                                                                                bool broadcast);

    /**
     * Change development committee quorum for excluding member
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_change_exclude_member_quorum(const std::string& creator,
                                                                                    uint64_t quorum_percent,
                                                                                    uint32_t lifetime_sec,
                                                                                    bool broadcast);

    /**
     * Change development committee for changing add/exclude quorum
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_change_base_quorum(const std::string& creator,
                                                                          uint64_t quorum_percent,
                                                                          uint32_t lifetime_sec,
                                                                          bool broadcast);

    /**
     * Change development committee for changing add/exclude quorum
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_change_transfer_quorum(const std::string& creator,
                                                                              uint64_t quorum_percent,
                                                                              uint32_t lifetime_sec,
                                                                              bool broadcast);

    /**
     * Change development committee for changing top budget amount quorum
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_change_budget_auction_properties_quorum(
        const std::string& creator, uint64_t quorum_percent, uint32_t lifetime_sec, bool broadcast);

    /**
     * Change development committee for changing advertising moderator quorum
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_change_advertising_moderator_quorum(const std::string& creator,
                                                                                           uint64_t quorum_percent,
                                                                                           uint32_t lifetime_sec,
                                                                                           bool broadcast);
    /**
     * Change development committee for changing betting moderator quorum
     *
     * @param creator
     * @param quorum_percent
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_change_betting_moderator_quorum(const std::string& creator,
                                                                                       uint64_t quorum_percent,
                                                                                       uint32_t lifetime_sec,
                                                                                       bool broadcast);

    /**
     * Create proposal for set up the advertising moderator.
     *
     * @param initiator
     * @param moderator
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_empower_advertising_moderator(const std::string& initiator,
                                                                                     const std::string& moderator,
                                                                                     uint32_t lifetime_sec,
                                                                                     bool broadcast);

    /**
     * Create proposal for set up the betting moderator.
     *
     * @param initiator
     * @param moderator
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_empower_betting_moderator(const std::string& initiator,
                                                                                 const std::string& moderator,
                                                                                 uint32_t lifetime_sec,
                                                                                 bool broadcast);

    /**
     * Create proposal for changing delay after game was finished before bets
     * will be resolved.
     *
     * @param initiator
     * @param delay_sec
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_committee_change_betting_resolve_delay(const std::string& initiator,
                                                                                    uint32_t delay_sec,
                                                                                    uint32_t lifetime_sec,
                                                                                    bool broadcast);

    /**
     * Create proposal for transfering SCR from development pool to account
     *
     * @param initiator
     * @param to_account
     * @param amount
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_pool_transfer(const std::string& initiator,
                                                           const std::string& to_account,
                                                           asset amount,
                                                           uint32_t lifetime_sec,
                                                           bool broadcast);

    /**
     * Create proposal for set up a vesting withdraw request.
     *
     * @param initiator
     * @param amount
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_pool_withdraw_vesting(const std::string& initiator,
                                                                   asset amount,
                                                                   uint32_t lifetime_sec,
                                                                   bool broadcast);

    /**
     * Create proposal for set up a post budgets auction properies.
     *
     * @param initiator
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_pool_post_budgets_auction_properties(const std::string& initiator,
                                                                                  const std::vector<percent_type>&,
                                                                                  uint32_t lifetime_sec,
                                                                                  bool broadcast);

    /**
     * Create proposal for set up a top budgets auction properies.
     *
     * @param initiator
     * @param lifetime_sec
     * @param broadcast
     */
    annotated_signed_transaction development_pool_banner_budgets_auction_properties(const std::string& initiator,
                                                                                    const std::vector<percent_type>&,
                                                                                    uint32_t lifetime_sec,
                                                                                    bool broadcast);

    /**
     * Get development committee
     */
    development_committee_api_obj get_development_committee();

    /**
     * Devcommittee operations have sequence numbers from 0 to N where N is the
     * most recent operation.
     *
     * This method returns operations in the range [from-limit, from]
     *
     * @param from the absolute sequence number, -1 means most recent, limit is
     *      the number of operations before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     */
    std::vector<applied_operation> get_devcommittee_history(uint64_t from, uint32_t limit);

    /**
     * Devcommittee operations have sequence numbers from 0 to N where N is the
     * most recent operation.
     *
     * This method returns operations in the range [from-limit, from]
     *
     * @param from the absolute sequence number, -1 means most recent, limit is
     *      the number of operations before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     */
    std::vector<applied_operation> get_devcommittee_scr_to_scr_transfers(uint64_t from, uint32_t limit);

    /**
     * Devcommittee operations have sequence numbers from 0 to N where N is the
     * most recent operation.
     *
     * This method returns operations in the range [from-limit, from]
     *
     * @param from the absolute sequence number, -1 means most recent, limit is
     *      the number of operations before from.
     * @param limit the maximum number of items that can be queried (0 to 100],
     *      must be less than from
     */
    std::vector<applied_withdraw_operation> get_devcommittee_sp_to_scr_transfers(uint64_t from, uint32_t limit);

    /// @}

    /// @brief Wallet Atomic Swap API
    /// @defgroup wallet_atomic_swap_api Atomic Swap
    /// @ingroup wallet_api
    /// @{

    /**
     * Initiating Atomic Swap transfer from initiator to participant.
     *
     * Asset (amount) will be locked for 48 hours while is not redeemed or
     * refund automatically by timeout.
     *
     * @warning API prints secret string to memorize.
     *          API prints secret hash as well.
     *
     * @param initiator the new contract owner
     * @param participant
     * @param amount SCR to transfer
     * @param metadata the additional contract info (obligations, courses)
     * @param secret_length the length of secret in bytes or 0 to choose length
     *      randomly
     * @param broadcast true if you wish to broadcast the transaction
     */
    atomicswap_contract_result_api_obj atomicswap_initiate(const std::string& initiator,
                                                           const std::string& participant,
                                                           const asset& amount,
                                                           const std::string& metadata,
                                                           const uint8_t secret_length,
                                                           bool broadcast);

    /**
     * Initiating Atomic Swap transfer from participant to initiator.
     *
     * Asset (amount) will be locked for 24 hours while is not redeemed or
     * refund before redeem.
     *
     * @warning The secret hash is obtained from atomicswap_initiate operation.
     *
     * @param secret_hash the secret hash (received from initiator)
     * @param participant the new contract owner
     * @param initiator
     * @param amount SCR to transfer
     * @param metadata the additional contract info (obligations, courses)
     * @param broadcast true if you wish to broadcast the transaction
     */
    atomicswap_contract_result_api_obj atomicswap_participate(const std::string& secret_hash,
                                                              const std::string& participant,
                                                              const std::string& initiator,
                                                              const asset& amount,
                                                              const std::string& metadata,
                                                              bool broadcast);

    /**
     * The Atomic Swap helper to get contract info.
     *
     * @param from the transfer 'from' address
     * @param to the transfer 'to' address
     * @param secret_hash the secret hash
     */
    atomicswap_contract_info_api_obj
    atomicswap_auditcontract(const std::string& from, const std::string& to, const std::string& secret_hash);

    /**
     * Redeeming Atomic Swap contract.
     *
     * This API transfers asset to participant balance and declassifies the
     * secret.
     *
     * @param from the transfer 'from' address
     * @param to the transfer 'to' address
     * @param secret the secret ("my secret") that was set in
     *      atomicswap_initiate
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    atomicswap_redeem(const std::string& from, const std::string& to, const std::string& secret, bool broadcast);

    /**
     * Extracting secret from participant contract if it is redeemed
     * by initiator.
     *
     * @param from the transfer 'from' address
     * @param to the transfer 'to' address
     * @param secret_hash the secret hash
     */
    std::string
    atomicswap_extractsecret(const std::string& from, const std::string& to, const std::string& secret_hash);

    /**
     * Refunding contact by participant.
     *
     * @warning Can't refund initiator contract. It is refunded automatically in
     * 48 hours.
     *
     * @param participant the refunded contract owner
     * @param initiator the initiator of Atomic Swap
     * @param secret_hash the secret hash (that was set in
     *      atomicswap_participate)
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction atomicswap_refund(const std::string& participant,
                                                   const std::string& initiator,
                                                   const std::string& secret_hash,
                                                   bool broadcast);

    /**
     * Atomic Swap helper to get list of contract info.
     *
     * @param owner
     */
    std::vector<atomicswap_contract_api_obj> get_atomicswap_contracts(const std::string& owner);

    /// @}

    /// @brief Wallet Betting API
    /// @defgroup wallet_betting_api Betting
    /// @ingroup wallet_api
    /// @{

    /**
     * Create betting game
     *
     * @param uuid game UUID
     * @param moderator betting moderator
     * @param json_metadata JSON metadata
     * @param start_time time when game should start
     * @param auto_resolve_delay_sec delay starting from @p start_time after
     *      expiration of which (if game results weren't posted) game will be
     *      cancelled
     * @param game type of game (soccer, hockey, etc.)
     * @param markets betting markets for this particular game
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_game(uuid_type uuid,
                                             account_name_type moderator,
                                             const std::string& json_metadata,
                                             fc::time_point_sec start_time,
                                             uint32_t auto_resolve_delay_sec,
                                             game_type game,
                                             const std::vector<market_type>& markets,
                                             bool broadcast);

    /**
     * Cancel game (return all bets to owners)
     *
     * @param uuid game UUID
     * @param moderator betting moderator
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction cancel_game(uuid_type uuid, account_name_type moderator, bool broadcast);

    /**
     * Update betting markets for the game
     *
     * @param uuid game UUID
     * @param moderator betting moderator
     * @param markets new betting markets for this game
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_game_markets(uuid_type uuid,
                                                     account_name_type moderator,
                                                     const std::vector<market_type>& markets,
                                                     bool broadcast);

    /**
     * Update game start time
     *
     * @param uuid game UUID
     * @param moderator betting moderator
     * @param start_time game new start time
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    update_game_start_time(uuid_type uuid, account_name_type moderator, fc::time_point_sec start_time, bool broadcast);

    /**
     * Post game results (provide won wincases)
     *
     * @param uuid game UUID
     * @param moderator betting moderator
     * @param wincases won wincases
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction post_game_results(uuid_type uuid,
                                                   account_name_type moderator,
                                                   const std::vector<wincase_type>& wincases,
                                                   bool broadcast);

    /**
     * Create bet
     *
     * @param uuid bet UUID
     * @param better owner for new bet
     * @param game_uuid game UUID which is specified during game creation
     * @param wincase wincase for bet
     * @param odds rational coefficient that define potential result (p).
     *      p = odds * stake
     * @param stake amount in SCR to bet
     * @param is_live specify is bet live or not
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction post_bet(uuid_type uuid,
                                          account_name_type better,
                                          uuid_type game_uuid,
                                          wincase_type wincase,
                                          odds_input odds,
                                          asset stake,
                                          bool is_live,
                                          bool broadcast);

    /**
     * Cancel pending bets list
     *
     * @param better owner
     * @param bet_uuids UUIDs of bets which are being cancelled
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    cancel_pending_bets(account_name_type better, const std::vector<uuid_type>& bet_uuids, bool broadcast);

    /**
     * Returns games
     *
     * @param filter [created, started, finished]
     * @return array of game_api_object's
     */
    std::vector<game_api_object> get_games_by_status(const fc::flat_set<game_status>& filter) const;

    /**
     * Returns games
     *
     * @param uuids UUIDs of games to return
     * @return array of game_api_object's
     */
    std::vector<game_api_object> get_games_by_uuids(const std::vector<uuid_type>& uuids) const;

    /**
     * Returns games
     *
     * @param from lower bound game id
     * @param limit query limit
     * @return array of game_api_object's
     */
    std::vector<game_api_object> lookup_games_by_id(game_id_type from, uint32_t limit) const;

    /**
     * Returns matched bets
     *
     * @param from lower bound bet id
     * @param limit query limit
     * @return array of matched_bet_api_object's
     */
    std::vector<matched_bet_api_object> lookup_matched_bets(matched_bet_id_type from, int64_t limit) const;

    /**
     * Return pending bets
     *
     * @param from lower bound bet id
     * @param limit query limit
     * @return array of pending_bet_api_object's
     */
    std::vector<pending_bet_api_object> lookup_pending_bets(pending_bet_id_type from, int64_t limit) const;

    /**
     * Returns matched bets
     *
     * @param uuids  The vector of UUIDs of pending bets which form matched bets
     * @return array of matched_bet_api_object's
     */
    std::vector<matched_bet_api_object> get_matched_bets(const std::vector<uuid_type>& uuids) const;

    /**
     * Return pending bets
     *
     * @param uuids The vector of UUIDs of pending bets which should be returned
     * @return array of pending_bet_api_object's
     */
    std::vector<pending_bet_api_object> get_pending_bets(const std::vector<uuid_type>& uuids) const;

    /**
     * Return betting properties
     *
     * @return betting property api object
     */
    betting_property_api_object get_betting_properties() const;

    /**
     * Returns bets with draw status
     *
     * @param game_uuid Game UUID
     * @return array of matched_bet_object's
     */
    std::vector<matched_bet_api_object> get_game_returns(const uuid_type& game_uuid) const;

    /**
     * Returns all winners for particular game
     *
     * @param game_uuid Game UUID
     * @return array of winner_api_object's
     */
    std::vector<winner_api_object> get_game_winners(const uuid_type& game_uuid) const;

    /**
     * Returns matched bets for game
     *
     * @param uuid Game UUID
     * @return array of matched_bet_api_object's
     */
    std::vector<matched_bet_api_object> get_game_matched_bets(const uuid_type& uuid) const;

    /**
     * This method get list of pending bets
     *
     * @param uuid Game UUID
     * @return array of pending_bet_api_object's
     */
    std::vector<pending_bet_api_object> get_game_pending_bets(const uuid_type& uuid) const;

    /// @}

    /// @brief Wallet NFT API
    /// @defgroup wallet_nft_api NFT
    /// @ingroup wallet_api
    /// @{

    /**
     * This method will create new NFT object
     *
     * @param owner account creating the new NFT object
     * @param uuid uuid of the new NFT object
     * @param name name of the new NFT object
     * @param initial_power initial power of the new NFT object
     * @param json_meta JSON metadata associated with the new NFT object
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_nft(const std::string& owner,
                                            const uuid_type& uuid,
                                            const std::string& name,
                                            int32_t initial_power,
                                            const std::string& json_meta,
                                            bool broadcast) const;

    /**
     * This method will update json metadata of an existing NFT object
     *
     * @param moderator moderator account
     * @param uuid uuid of the NFT object
     * @param json_meta JSON metadata associated with the NFT
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_nft_meta(const std::string& moderator,
                                                 const uuid_type& uuid,
                                                 const std::string& json_meta,
                                                 bool broadcast) const;

    /**
     * This method will adjust experience of an existing NFT object
     *
     * @param moderator moderator account
     * @param uuid uuid of the NFT object
     * @param experience experience
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction adjust_nft_experience(const std::string& moderator,
                                                       const uuid_type& uuid,
                                                       int32_t experience,
                                                       bool broadcast) const;

    /**
     * This method will update name of an existing NFT object
     *
     * @param moderator moderator account
     * @param uuid uuid of the NFT object
     * @param name name associated with the NFT
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_nft_name(const std::string& moderator,
                                                 const uuid_type& uuid,
                                                 const std::string& name,
                                                 bool broadcast) const;
    /**
     * Get NFT object by id
     *
     * @param id id of the NFT object
     * @return nft_api_obj
     */
    nft_api_obj get_nft_by_id(int64_t id) const;

    /**
     * Get NFT object by name
     *
     * @param name name of the NFT object
     * @return nft_api_obj
     */
    nft_api_obj get_nft_by_name(const account_name_type& name) const;

    /**
     * Get NFT object by uuid
     *
     * @param uuid UUID of the NFT object
     * @return nft_api_obj
     */
    nft_api_obj get_nft_by_uuid(const uuid_type& uuid) const;

    /**
     * Retrieve list of NFT objects  in range [from, from+limit-1]
     *
     * @param from lower bound NFT id
     * @param limit limit number of objects in response
     * @return std::vector<nft_api_obj>
     */
    std::vector<nft_api_obj> lookup_nft(int64_t from, uint32_t limit) const;

    /**
     * Create a new game round object
     *
     * @param owner account name of the game round owner
     * @param uuid UUID of the new game round
     * @param verification_key verification key
     * @param seed seed for random generator
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_game_round(const std::string& owner,
                                                   const uuid_type& uuid,
                                                   const std::string& verification_key,
                                                   const std::string& seed,
                                                   bool broadcast) const;

    /**
     * This method will update game round result
     *
     * @param owner account name of the game round owner
     * @param uuid UUID of the game round
     * @param proof VRF proof
     * @param vrf VRF
     * @param result result
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_game_round_result(const std::string& owner,
                                                   const uuid_type& uuid,
                                                   const std::string& proof,
                                                   const std::string& vrf,
                                                   int32_t result,
                                                   bool broadcast) const;

    /**
     * Get game round by uuid object
     *
     * @param uuid UUID of the game round
     * @return game_round_api_obj
     */
    game_round_api_obj get_game_round_by_uuid(const uuid_type& uuid) const;

    /**
     * Retrieve list of game rounds in range [from, from+limit-1]
     *
     * @param from lower bound game round id
     * @param limit limit number of objects in response
     * @return list of game round's
     */
    std::vector<game_round_api_obj> lookup_game_round(int64_t from, uint32_t limit) const;

    /// @}

public:
    fc::signal<void(bool)> lock_changed;

private:
    std::shared_ptr<detail::wallet_api_impl> my;
    exit_func_type exit_func;
};

struct plain_keys
{
    fc::sha512 checksum;
    std::map<public_key_type, std::string> keys;
};
} // namespace wallet
} // namespace scorum

// clang-format off

FC_REFLECT(scorum::wallet::wallet_data,
           (cipher_keys)
           (ws_server)
           (ws_user)
           (ws_password)
           (chain_id))

FC_REFLECT( scorum::wallet::brain_key_info, (brain_priv_key)(wif_priv_key) (pub_key))

FC_REFLECT( scorum::wallet::plain_keys, (checksum)(keys) )

FC_REFLECT_ENUM( scorum::wallet::authority_type, (owner)(active)(posting) )

FC_API( scorum::wallet::wallet_api,
        /// wallet api
        (help)(gethelp)
        (about)(is_new)(is_locked)(lock)(unlock)(set_password)
        (load_wallet_file)(save_wallet_file)(exit)

        /// key api
        (import_key)
        (suggest_brain_key)
        (list_keys)
        (get_private_key)
        (get_private_key_from_password)
        (normalize_brain_key)

        /// block/transaction/operation history
        (get_block_header)
        (get_block)
        (get_block_headers_history)
        (get_blocks_history)
        (get_ops_in_block)
        (get_ops_history)
        (get_ops_history_by_time)
        (get_transaction)

        /// query api
        (info)
        (get_chain_capital)
        (get_active_witnesses)
        (list_my_accounts)
        (list_accounts)
        (list_witnesses)
        (get_witness)
        (get_account)
        (get_account_balance)
        (get_account_history)
        (get_account_scr_to_scr_transfers)
        (get_account_scr_to_sp_transfers)
        (get_account_sp_to_scr_transfers)
        (get_withdraw_routes)
        (get_owner_history)
        (get_encrypted_memo)

        (create_account)
        (create_account_with_keys)
        (create_account_delegated)
        (create_account_with_keys_delegated)
        (create_account_by_committee)
        (create_account_by_committee_with_keys)
        (update_account)
        (update_account_auth_key)
        (update_account_auth_account)
        (update_account_auth_threshold)
        (update_account_meta)
        (update_account_memo_key)
        (delegate_scorumpower)
        (delegate_scorumpower_from_reg_pool)
        (update_witness)
        (set_voting_proxy)
        (vote_for_witness)
        (transfer)
        (escrow_transfer)
        (escrow_approve)
        (escrow_dispute)
        (escrow_release)
        (transfer_to_scorumpower)
        (withdraw_scorumpower)
        (set_withdraw_scorumpower_route)
        (post_comment)
        (vote)
        (set_transaction_expiration)
        (challenge)
        (prove)
        (request_account_recovery)
        (recover_account)
        (change_recovery_account)
        (decrypt_memo)
        (decline_voting_rights)

        /// Advertising
        (create_budget_for_post)
        (create_budget_for_banner)
        (update_budget_for_banner)
        (update_budget_for_post)
        (close_budget_for_post)
        (close_budget_for_banner)
        (close_budget_for_post_by_moderator)
        (close_budget_for_banner_by_moderator)
        (list_my_budgets)
        (list_post_budget_owners)
        (list_banner_budget_owners)
        (get_post_budgets)
        (get_banner_budgets)

        /// Registration committee api
        (vote_for_committee_proposal)
        (registration_committee_add_member)
        (registration_committee_exclude_member)
        (list_registration_committee)
        (registration_committee_change_add_member_quorum)
        (registration_committee_change_exclude_member_quorum)
        (registration_committee_change_base_quorum)
        (get_registration_committee)
        (list_proposals)
        (get_devcommittee_history)
        (get_devcommittee_scr_to_scr_transfers)
        (get_devcommittee_sp_to_scr_transfers)

        /// Development committee api
        (development_committee_add_member)
        (development_committee_exclude_member)
        (list_development_committee)
        (development_committee_change_add_member_quorum)
        (development_committee_change_exclude_member_quorum)
        (development_committee_change_base_quorum)
        (development_committee_change_transfer_quorum)
        (development_committee_change_budget_auction_properties_quorum)
        (development_committee_change_advertising_moderator_quorum)
        (development_committee_change_betting_moderator_quorum)
        (development_committee_empower_advertising_moderator)
        (development_committee_empower_betting_moderator)
        (development_committee_change_betting_resolve_delay)
        (get_development_committee)
        (development_pool_transfer)
        (development_pool_withdraw_vesting)
        (development_pool_post_budgets_auction_properties)
        (development_pool_banner_budgets_auction_properties)

        /// Atomic Swap API
        (atomicswap_initiate)
        (atomicswap_participate)
        (atomicswap_redeem)
        (atomicswap_auditcontract)
        (atomicswap_extractsecret)
        (atomicswap_refund)
        (get_atomicswap_contracts)

        /// betting API
        (create_game)
        (cancel_game)
        (update_game_markets)
        (update_game_start_time)
        (post_game_results)
        (post_bet)
        (cancel_pending_bets)

        (get_games_by_status)
        (get_games_by_uuids)
        (lookup_games_by_id)
        (lookup_matched_bets)
        (lookup_pending_bets)
        (get_matched_bets)
        (get_pending_bets)
        (get_betting_properties)
        (get_game_returns)
        (get_game_winners)
        (get_game_matched_bets)
        (get_game_pending_bets)

        /// nft
        (create_nft)
        (update_nft_meta)
        (adjust_nft_experience)
        (update_nft_name)
        (get_nft_by_id)
        (get_nft_by_name)
        (get_nft_by_uuid)
        (lookup_nft)

        (create_game_round)
        (update_game_round_result)
        (get_game_round_by_uuid)
        (lookup_game_round)

        /// helper api
        (get_prototype_operation)
        (serialize_transaction)
        (sign_transaction)

        (network_add_nodes)
        (network_get_connected_peers)
      )

FC_REFLECT( scorum::wallet::memo_data, (from)(to)(nonce)(check)(encrypted) )

// clang-format on
