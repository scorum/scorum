#pragma once
#include <scorum/app/applied_operation.hpp>
#include <scorum/app/state.hpp>

#include <scorum/chain/database.hpp>
#include <scorum/chain/scorum_objects.hpp>
#include <scorum/chain/scorum_object_types.hpp>
#include <scorum/chain/history_object.hpp>

#include <scorum/tags/tags_plugin.hpp>

#include <scorum/follow/follow_plugin.hpp>
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

struct scheduled_hardfork
{
    hardfork_version hf_version;
    fc::time_point_sec live_time;
};

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
 *  Defines the arguments to a query as a struct so it can be easily extended
 */
struct discussion_query
{
    void validate() const
    {
        FC_ASSERT(filter_tags.find(tag) == filter_tags.end());
        FC_ASSERT(limit <= 100);
    }

    std::string tag;
    uint32_t limit = 0;
    std::set<std::string> filter_tags;
    std::set<std::string> select_authors; ///< list of authors to include, posts not by this author are filtered
    std::set<std::string> select_tags; ///< list of tags to include, posts without these tags are filtered
    uint32_t truncate_body = 0; ///< the number of bytes of the post body to return, 0 for all
    optional<std::string> start_author;
    optional<std::string> start_permlink;
    optional<std::string> parent_author;
    optional<std::string> parent_permlink;
};

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

    std::vector<tag_api_obj> get_trending_tags(const std::string& after_tag, uint32_t limit) const;

    /**
     *  This API is a short-cut for returning all of the state required for a particular URL
     *  with a single query.
     */
    state get_state(std::string path) const;

    std::vector<account_name_type> get_active_witnesses() const;

    /////////////////////////////
    // Blocks and transactions //
    /////////////////////////////

    /**
     * @brief Retrieve a block header
     * @param block_num Height of the block whose header should be returned
     * @return header of the referenced block, or null if no matching block was found
     */
    optional<block_header> get_block_header(uint32_t block_num) const;

    /**
     * @brief Retrieve a full, signed block
     * @param block_num Height of the block to be returned
     * @return the referenced block, or null if no matching block was found
     */
    optional<signed_block_api_obj> get_block(uint32_t block_num) const;

    /**
     *  @brief Get sequence of operations included/generated within a particular block
     *  @param block_num Height of the block whose generated virtual operations should be returned
     *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
     *  @return sequence of operations included/generated within the block
     */
    std::vector<applied_operation> get_ops_in_block(uint32_t block_num, bool only_virtual = true) const;

    /////////////
    // Globals //
    /////////////

    /**
     * @brief Retrieve compile-time constants
     */
    fc::variant_object get_config() const;

    /**
     * @brief Retrieve the current @ref dynamic_global_property_object
     */
    dynamic_global_property_api_obj get_dynamic_global_properties() const;
    chain_properties get_chain_properties() const;
    witness_schedule_api_obj get_witness_schedule() const;
    hardfork_version get_hardfork_version() const;
    scheduled_hardfork get_next_scheduled_hardfork() const;
    reward_fund_api_obj get_reward_fund() const;

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

    std::vector<budget_api_obj> get_budgets(const std::set<std::string>& account_names) const;

    std::set<std::string> lookup_budget_owners(const std::string& lower_bound_name, uint32_t limit) const;

    std::vector<owner_authority_history_api_obj> get_owner_history(const std::string& account) const;

    optional<account_recovery_request_api_obj> get_recovery_request(const std::string& account) const;

    optional<escrow_api_obj> get_escrow(const std::string& from, uint32_t escrow_id) const;

    std::vector<withdraw_route> get_withdraw_routes(const std::string& account,
                                                    withdraw_route_type type = outgoing) const;

    optional<account_bandwidth_api_obj> get_account_bandwidth(const std::string& account,
                                                              witness::bandwidth_type type) const;

    std::vector<vesting_delegation_api_obj>
    get_vesting_delegations(const std::string& account, const std::string& from, uint32_t limit = 100) const;
    std::vector<vesting_delegation_expiration_api_obj>
    get_expiring_vesting_delegations(const std::string& account, time_point_sec from, uint32_t limit = 100) const;

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
    annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;

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

    discussion get_content(const std::string& author, const std::string& permlink) const;
    std::vector<discussion> get_content_replies(const std::string& parent, const std::string& parent_permlink) const;

    ///@{ tags API
    /** This API will return the top 1000 tags used by an author sorted by most frequently used */
    std::vector<std::pair<std::string, uint32_t>> get_tags_used_by_author(const std::string& author) const;
    std::vector<discussion> get_discussions_by_payout(const discussion_query& query) const;
    std::vector<discussion> get_post_discussions_by_payout(const discussion_query& query) const;
    std::vector<discussion> get_comment_discussions_by_payout(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_trending(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_created(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_active(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_cashout(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_votes(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_children(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_hot(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_feed(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_blog(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_comments(const discussion_query& query) const;
    std::vector<discussion> get_discussions_by_promoted(const discussion_query& query) const;

    ///@}

    /**
     *  For each of these filters:
     *     Get root content...
     *     Get any content...
     *     Get root content in category..
     *     Get any content in category...
     *
     *  Return discussions
     *     Total Discussion Pending Payout
     *     Last Discussion Update (or reply)... think
     *     Top Discussions by Total Payout
     *
     *  Return content (comments)
     *     Pending Payout Amount
     *     Pending Payout Time
     *     Creation Date
     *
     */
    ///@{

    /**
     *  Return the active discussions with the highest cumulative pending payouts without respect to category, total
     *  pending payout means the pending payout of all children as well.
     */
    std::vector<discussion>
    get_replies_by_last_update(account_name_type start_author, const std::string& start_permlink, uint32_t limit) const;

    /**
     *  This method is used to fetch all posts/comments by start_author that occur after before_date and start_permlink
     * with up to limit being returned.
     *
     *  If start_permlink is empty then only before_date will be considered. If both are specified the earlier to the
     * two metrics will be used. This
     *  should allow easy pagination.
     */
    std::vector<discussion> get_discussions_by_author_before_date(const std::string& author,
                                                                  const std::string& start_permlink,
                                                                  time_point_sec before_date,
                                                                  uint32_t limit) const;

    /**
     *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
     *  returns operations in the range [from-limit, from]
     *
     *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
     *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
     */
    std::map<uint32_t, applied_operation>
    get_account_history(const std::string& account, uint64_t from, uint32_t limit) const;

    ////////////////////////////
    // Handlers - not exposed //
    ////////////////////////////
    void on_api_startup();

private:
    void set_pending_payout(discussion& d) const;
    void set_url(discussion& d) const;
    discussion get_discussion(comment_id_type, uint32_t truncate_body = 0) const;

    static bool filter_default(const comment_api_obj& c)
    {
        return false;
    }
    static bool exit_default(const comment_api_obj& c)
    {
        return false;
    }
    static bool tag_exit_default(const tags::tag_object& c)
    {
        return false;
    }

    template <typename Index, typename StartItr>
    std::vector<discussion>
    get_discussions(const discussion_query& q,
                    const std::string& tag,
                    comment_id_type parent,
                    const Index& idx,
                    StartItr itr,
                    uint32_t truncate_body = 0,
                    const std::function<bool(const comment_api_obj&)>& filter = &database_api::filter_default,
                    const std::function<bool(const comment_api_obj&)>& exit = &database_api::exit_default,
                    const std::function<bool(const tags::tag_object&)>& tag_exit = &database_api::tag_exit_default,
                    bool ignore_parent = false) const;
    comment_id_type get_parent(const discussion_query& q) const;

    void recursively_fetch_content(state& _state, discussion& root, std::set<std::string>& referenced_accounts) const;

    std::shared_ptr<database_api_impl> my;
};
} // namespace app
} // namespace scorum

// clang-format off

FC_REFLECT( scorum::app::scheduled_hardfork, (hf_version)(live_time) )
FC_REFLECT( scorum::app::withdraw_route, (from_account)(to_account)(percent)(auto_vest) )

FC_REFLECT( scorum::app::discussion_query, (tag)(filter_tags)(select_tags)(select_authors)(truncate_body)(start_author)(start_permlink)(parent_author)(parent_permlink)(limit) )

FC_REFLECT_ENUM( scorum::app::withdraw_route_type, (incoming)(outgoing)(all) )

FC_API(scorum::app::database_api,
   // Subscriptions
   (set_block_applied_callback)

   // tags
   (get_trending_tags)
   (get_tags_used_by_author)
   (get_discussions_by_payout)
   (get_post_discussions_by_payout)
   (get_comment_discussions_by_payout)
   (get_discussions_by_trending)
   (get_discussions_by_created)
   (get_discussions_by_active)
   (get_discussions_by_cashout)
   (get_discussions_by_votes)
   (get_discussions_by_children)
   (get_discussions_by_hot)
   (get_discussions_by_feed)
   (get_discussions_by_blog)
   (get_discussions_by_comments)
   (get_discussions_by_promoted)

   // Blocks and transactions
   (get_block_header)
   (get_block)
   (get_ops_in_block)
   (get_state)

   // Globals
   (get_config)
   (get_dynamic_global_properties)
   (get_chain_properties)
   (get_witness_schedule)
   (get_hardfork_version)
   (get_next_scheduled_hardfork)
   (get_reward_fund)

   // Keys
   (get_key_references)

   // Accounts
   (get_accounts)
   (get_account_references)
   (lookup_account_names)
   (lookup_accounts)
   (get_account_count)
   (get_account_history)
   (get_owner_history)
   (get_recovery_request)
   (get_escrow)
   (get_withdraw_routes)
   (get_account_bandwidth)
   (get_vesting_delegations)
   (get_expiring_vesting_delegations)

   // Authority / validation
   (get_transaction_hex)
   (get_transaction)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)

   // votes
   (get_active_votes)
   (get_account_votes)

   // content
   (get_content)
   (get_content_replies)
   (get_discussions_by_author_before_date)
   (get_replies_by_last_update)

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
)

// clang-format on
