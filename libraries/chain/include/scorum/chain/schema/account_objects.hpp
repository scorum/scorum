#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <scorum/protocol/authority.hpp>
#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/chain/schema/witness_objects.hpp>
#include <scorum/chain/schema/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

namespace scorum {
namespace chain {

using scorum::protocol::authority;
using scorum::protocol::percent_type;

// clang-format off
class account_object : public object<account_object_type, account_object>
{
public:
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(account_object, (json_metadata))

    id_type id;

    account_name_type name;
    public_key_type memo_key;
    fc::shared_string json_metadata;
    account_name_type proxy;

    time_point_sec last_account_update;

    time_point_sec created;
    bool created_by_genesis = false;
    bool owner_challenged   = false;
    bool active_challenged  = false;
    time_point_sec last_owner_proved    = time_point_sec::min();
    time_point_sec last_active_proved   = time_point_sec::min();
    account_name_type recovery_account;
    time_point_sec last_account_recovery;

    bool can_vote = true;
    ///current voting power of this account, it falls after every vote
    percent_type voting_power = SCORUM_100_PERCENT;
    ///used to increase the voting power of this account the longer it goes without voting.
    time_point_sec last_vote_time;
    ///when voting powert will be (or was) restored
    time_point_sec voting_power_restoring_time;
    ///SP when account voted to calculate account reward weight
    asset vote_reward_competitive_sp = asset(0, SP_SYMBOL);

    asset balance = asset(0, SCORUM_SYMBOL);    ///< total liquid shares held by this account

    asset scorumpower =              asset(0, SP_SYMBOL); ///< total scorumpower (SP) held by this account, controls its voting power
    asset delegated_scorumpower =    asset(0, SP_SYMBOL);
    asset received_scorumpower =     asset(0, SP_SYMBOL);

    fc::array<share_type, SCORUM_MAX_PROXY_RECURSION_DEPTH> proxied_vsf_votes; // = std::vector<share_type>(SCORUM_MAX_PROXY_RECURSION_DEPTH, 0 );
                                                                               ///< the total VFS votes proxied to this account

    uint16_t witnesses_voted_for = 0;

    time_point_sec last_post;
    time_point_sec last_root_post = fc::time_point_sec::min();

    time_point_sec active_sp_holders_cashout_time = fc::time_point_sec::maximum();
    asset active_sp_holders_pending_scr_reward = asset(0, SCORUM_SYMBOL);
    asset active_sp_holders_pending_sp_reward = asset(0, SP_SYMBOL);

    /// This function should be used only when the account votes for a witness directly
    share_type witness_vote_weight() const
    {
        return std::accumulate(proxied_vsf_votes.begin(), proxied_vsf_votes.end(), scorumpower.amount);
    }
    share_type proxied_vsf_votes_total() const
    {
        return std::accumulate(proxied_vsf_votes.begin(), proxied_vsf_votes.end(), share_type());
    }

    asset effective_scorumpower() const
    {
        return scorumpower - delegated_scorumpower + received_scorumpower;
    }
};
// clang-format on

class account_blogging_statistic_object
    : public object<account_blogging_statistic_object_type, account_blogging_statistic_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(account_blogging_statistic_object)

    id_type id;

    account_id_type account;

    uint32_t post_count = 0;
    uint32_t comment_count = 0;
    uint32_t vote_count = 0;

    asset curation_rewards_scr = asset(0, SCORUM_SYMBOL);
    asset curation_rewards_sp = asset(0, SP_SYMBOL);

    asset posting_rewards_scr = asset(0, SCORUM_SYMBOL);
    asset posting_rewards_sp = asset(0, SP_SYMBOL);
};

class account_authority_object : public object<account_authority_object_type, account_authority_object>
{
public:
    /// \cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(account_authority_object, (owner)(active)(posting))

    id_type id;

    account_name_type account;

    shared_authority owner; ///< used for backup control, can set owner or active
    shared_authority active; ///< used for all monetary operations, can set active or posting
    shared_authority posting; ///< used for voting and posting

    time_point_sec last_owner_update;
};

class scorumpower_delegation_object : public object<scorumpower_delegation_object_type, scorumpower_delegation_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(scorumpower_delegation_object)

    id_type id;
    account_name_type delegator;
    account_name_type delegatee;
    asset scorumpower = asset(0, SP_SYMBOL);
    time_point_sec min_delegation_time;
};

class reg_pool_sp_delegation_object : public object<reg_pool_sp_delegation_object_type, reg_pool_sp_delegation_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(reg_pool_sp_delegation_object)

    id_type id;
    account_name_type delegatee;
    asset sp = asset(0, SP_SYMBOL);
};

class scorumpower_delegation_expiration_object
    : public object<scorumpower_delegation_expiration_object_type, scorumpower_delegation_expiration_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(scorumpower_delegation_expiration_object)

    id_type id;
    account_name_type delegator;
    asset scorumpower = asset(0, SP_SYMBOL);
    time_point_sec expiration;
};

class owner_authority_history_object
    : public object<owner_authority_history_object_type, owner_authority_history_object>
{
public:
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(owner_authority_history_object, (previous_owner_authority))

    id_type id;

    account_name_type account;
    shared_authority previous_owner_authority;
    time_point_sec last_valid_time;
};

class account_recovery_request_object
    : public object<account_recovery_request_object_type, account_recovery_request_object>
{
public:
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(account_recovery_request_object, (new_owner_authority))

    id_type id;

    account_name_type account_to_recover;
    shared_authority new_owner_authority;
    time_point_sec expires;
};

class change_recovery_account_request_object
    : public object<change_recovery_account_request_object_type, change_recovery_account_request_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(change_recovery_account_request_object)

    id_type id;

    account_name_type account_to_recover;
    account_name_type recovery_account;
    time_point_sec effective_on;
};

class account_registration_bonus_object
    : public object<account_registration_bonus_object_type, account_registration_bonus_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(account_registration_bonus_object)

    id_type id;

    account_name_type account;

    asset bonus = asset(0, SP_SYMBOL);

    time_point_sec expires;
};

struct by_name;
struct by_proxy;
struct by_last_post;
struct by_scorum_balance;
struct by_smp_balance;
struct by_created_by_genesis;
struct by_voting_power_restoring_time;
struct by_active_sp_holders_cashout_time;

/**
 * @ingroup object_index
 */
typedef shared_multi_index_container<account_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<account_object,
                                                                      account_id_type,
                                                                      &account_object::id>>,
                                                ordered_unique<tag<by_name>,
                                                               member<account_object,
                                                                      account_name_type,
                                                                      &account_object::name>>,
                                                ordered_non_unique<tag<by_created_by_genesis>,
                                                                   member<account_object,
                                                                          bool,
                                                                          &account_object::created_by_genesis>>,
                                                ordered_unique<tag<by_proxy>,
                                                               composite_key<account_object,
                                                                             member<account_object,
                                                                                    account_name_type,
                                                                                    &account_object::proxy>,
                                                                             member<account_object,
                                                                                    account_id_type,
                                                                                    &account_object::id>> /// composite
                                                               /// key by
                                                               /// proxy
                                                               >,
                                                ordered_unique<tag<by_last_post>,
                                                               composite_key<account_object,
                                                                             member<account_object,
                                                                                    time_point_sec,
                                                                                    &account_object::last_post>,
                                                                             member<account_object,
                                                                                    account_id_type,
                                                                                    &account_object::id>>,
                                                               composite_key_compare<std::greater<time_point_sec>,
                                                                                     std::less<account_id_type>>>,
                                                ordered_unique<tag<by_scorum_balance>,
                                                               composite_key<account_object,
                                                                             member<account_object,
                                                                                    asset,
                                                                                    &account_object::balance>,
                                                                             member<account_object,
                                                                                    account_id_type,
                                                                                    &account_object::id>>,
                                                               composite_key_compare<std::greater<asset>,
                                                                                     std::less<account_id_type>>>,
                                                ordered_unique<tag<by_smp_balance>,
                                                               composite_key<account_object,
                                                                             member<account_object,
                                                                                    asset,
                                                                                    &account_object::scorumpower>,
                                                                             member<account_object,
                                                                                    account_id_type,
                                                                                    &account_object::id>>,
                                                               composite_key_compare<std::greater<asset>,
                                                                                     std::less<account_id_type>>>,
                                                ordered_non_unique<tag<by_voting_power_restoring_time>,
                                                                   member<account_object,
                                                                          time_point_sec,
                                                                          &account_object::
                                                                              voting_power_restoring_time>>,
                                                ordered_unique<tag<by_active_sp_holders_cashout_time>,
                                                               composite_key<account_object,
                                                                             member<account_object,
                                                                                    time_point_sec,
                                                                                    &account_object::
                                                                                        active_sp_holders_cashout_time>,
                                                                             member<account_object,
                                                                                    account_id_type,
                                                                                    &account_object::id>>>>>
    account_index;

struct by_account_id;
struct by_post_count;
struct by_vote_count;

typedef shared_multi_index_container<account_blogging_statistic_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<account_blogging_statistic_object,
                                                                      account_blogging_statistic_id_type,
                                                                      &account_blogging_statistic_object::id>>,
                                                ordered_unique<tag<by_account_id>,
                                                               member<account_blogging_statistic_object,
                                                                      account_id_type,
                                                                      &account_blogging_statistic_object::account>>,
                                                ordered_unique<tag<by_post_count>,
                                                               composite_key<account_blogging_statistic_object,
                                                                             member<account_blogging_statistic_object,
                                                                                    uint32_t,
                                                                                    &account_blogging_statistic_object::
                                                                                        post_count>,
                                                                             member<account_blogging_statistic_object,
                                                                                    account_id_type,
                                                                                    &account_blogging_statistic_object::
                                                                                        account>>,
                                                               composite_key_compare<std::greater<uint32_t>,
                                                                                     std::less<account_id_type>>>,
                                                ordered_unique<tag<by_vote_count>,
                                                               composite_key<account_blogging_statistic_object,
                                                                             member<account_blogging_statistic_object,
                                                                                    uint32_t,
                                                                                    &account_blogging_statistic_object::
                                                                                        vote_count>,
                                                                             member<account_blogging_statistic_object,
                                                                                    account_id_type,
                                                                                    &account_blogging_statistic_object::
                                                                                        account>>,
                                                               composite_key_compare<std::greater<uint32_t>,
                                                                                     std::less<account_id_type>>>>>
    account_blogging_statistic_index;

struct by_account;
struct by_last_valid;

typedef shared_multi_index_container<owner_authority_history_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<owner_authority_history_object,
                                                                      owner_authority_history_id_type,
                                                                      &owner_authority_history_object::id>>,
                                                ordered_unique<tag<by_account>,
                                                               composite_key<owner_authority_history_object,
                                                                             member<owner_authority_history_object,
                                                                                    account_name_type,
                                                                                    &owner_authority_history_object::
                                                                                        account>,
                                                                             member<owner_authority_history_object,
                                                                                    time_point_sec,
                                                                                    &owner_authority_history_object::
                                                                                        last_valid_time>,
                                                                             member<owner_authority_history_object,
                                                                                    owner_authority_history_id_type,
                                                                                    &owner_authority_history_object::
                                                                                        id>>,
                                                               composite_key_compare<std::less<account_name_type>,
                                                                                     std::less<time_point_sec>,
                                                                                     std::
                                                                                         less<owner_authority_history_id_type>>>>>
    owner_authority_history_index;

struct by_last_owner_update;

typedef shared_multi_index_container<account_authority_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<account_authority_object,
                                                                      account_authority_id_type,
                                                                      &account_authority_object::id>>,
                                                ordered_unique<tag<by_account>,
                                                               composite_key<account_authority_object,
                                                                             member<account_authority_object,
                                                                                    account_name_type,
                                                                                    &account_authority_object::account>,
                                                                             member<account_authority_object,
                                                                                    account_authority_id_type,
                                                                                    &account_authority_object::id>>,
                                                               composite_key_compare<std::less<account_name_type>,
                                                                                     std::
                                                                                         less<account_authority_id_type>>>,
                                                ordered_unique<tag<by_last_owner_update>,
                                                               composite_key<account_authority_object,
                                                                             member<account_authority_object,
                                                                                    time_point_sec,
                                                                                    &account_authority_object::
                                                                                        last_owner_update>,
                                                                             member<account_authority_object,
                                                                                    account_authority_id_type,
                                                                                    &account_authority_object::id>>,
                                                               composite_key_compare<std::greater<time_point_sec>,
                                                                                     std::
                                                                                         less<account_authority_id_type>>>>>
    account_authority_index;

struct by_delegation;

typedef shared_multi_index_container<scorumpower_delegation_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<scorumpower_delegation_object,
                                                                      scorumpower_delegation_id_type,
                                                                      &scorumpower_delegation_object::id>>,
                                                ordered_unique<tag<by_delegation>,
                                                               composite_key<scorumpower_delegation_object,
                                                                             member<scorumpower_delegation_object,
                                                                                    account_name_type,
                                                                                    &scorumpower_delegation_object::
                                                                                        delegator>,
                                                                             member<scorumpower_delegation_object,
                                                                                    account_name_type,
                                                                                    &scorumpower_delegation_object::
                                                                                        delegatee>>,
                                                               composite_key_compare<std::less<account_name_type>,
                                                                                     std::less<account_name_type>>>>>
    scorumpower_delegation_index;

struct by_delegatee;

typedef shared_multi_index_container<reg_pool_sp_delegation_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<reg_pool_sp_delegation_object,
                                                                      reg_pool_sp_delegation_id_type,
                                                                      &reg_pool_sp_delegation_object::id>>,
                                                hashed_non_unique<tag<by_delegatee>,
                                                                  member<reg_pool_sp_delegation_object,
                                                                         account_name_type,
                                                                         &reg_pool_sp_delegation_object::delegatee>>>>
    reg_pool_sp_delegation_index;

struct by_expiration;
struct by_account_expiration;

typedef shared_multi_index_container<scorumpower_delegation_expiration_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<scorumpower_delegation_expiration_object,
                                                                      scorumpower_delegation_expiration_id_type,
                                                                      &scorumpower_delegation_expiration_object::id>>,
                                                ordered_unique<tag<by_expiration>,
                                                               composite_key<scorumpower_delegation_expiration_object,
                                                                             member<scorumpower_delegation_expiration_object,
                                                                                    time_point_sec,
                                                                                    &scorumpower_delegation_expiration_object::
                                                                                        expiration>,
                                                                             member<scorumpower_delegation_expiration_object,
                                                                                    scorumpower_delegation_expiration_id_type,
                                                                                    &scorumpower_delegation_expiration_object::
                                                                                        id>>,
                                                               composite_key_compare<std::less<time_point_sec>,
                                                                                     std::
                                                                                         less<scorumpower_delegation_expiration_id_type>>>,
                                                ordered_unique<tag<by_account_expiration>,
                                                               composite_key<scorumpower_delegation_expiration_object,
                                                                             member<scorumpower_delegation_expiration_object,
                                                                                    account_name_type,
                                                                                    &scorumpower_delegation_expiration_object::
                                                                                        delegator>,
                                                                             member<scorumpower_delegation_expiration_object,
                                                                                    time_point_sec,
                                                                                    &scorumpower_delegation_expiration_object::
                                                                                        expiration>,
                                                                             member<scorumpower_delegation_expiration_object,
                                                                                    scorumpower_delegation_expiration_id_type,
                                                                                    &scorumpower_delegation_expiration_object::
                                                                                        id>>,
                                                               composite_key_compare<std::less<account_name_type>,
                                                                                     std::less<time_point_sec>,
                                                                                     std::
                                                                                         less<scorumpower_delegation_expiration_id_type>>>>>
    scorumpower_delegation_expiration_index;

typedef shared_multi_index_container<account_recovery_request_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<account_recovery_request_object,
                                                                      account_recovery_request_id_type,
                                                                      &account_recovery_request_object::id>>,
                                                ordered_unique<tag<by_account>,
                                                               composite_key<account_recovery_request_object,
                                                                             member<account_recovery_request_object,
                                                                                    account_name_type,
                                                                                    &account_recovery_request_object::
                                                                                        account_to_recover>,
                                                                             member<account_recovery_request_object,
                                                                                    account_recovery_request_id_type,
                                                                                    &account_recovery_request_object::
                                                                                        id>>,
                                                               composite_key_compare<std::less<account_name_type>,
                                                                                     std::
                                                                                         less<account_recovery_request_id_type>>>,
                                                ordered_unique<tag<by_expiration>,
                                                               composite_key<account_recovery_request_object,
                                                                             member<account_recovery_request_object,
                                                                                    time_point_sec,
                                                                                    &account_recovery_request_object::
                                                                                        expires>,
                                                                             member<account_recovery_request_object,
                                                                                    account_recovery_request_id_type,
                                                                                    &account_recovery_request_object::
                                                                                        id>>,
                                                               composite_key_compare<std::less<time_point_sec>,
                                                                                     std::
                                                                                         less<account_recovery_request_id_type>>>>>
    account_recovery_request_index;

struct by_effective_date;

typedef shared_multi_index_container<change_recovery_account_request_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<change_recovery_account_request_object,
                                                                      change_recovery_account_request_id_type,
                                                                      &change_recovery_account_request_object::id>>,
                                                ordered_unique<tag<by_account>,
                                                               composite_key<change_recovery_account_request_object,
                                                                             member<change_recovery_account_request_object,
                                                                                    account_name_type,
                                                                                    &change_recovery_account_request_object::
                                                                                        account_to_recover>,
                                                                             member<change_recovery_account_request_object,
                                                                                    change_recovery_account_request_id_type,
                                                                                    &change_recovery_account_request_object::
                                                                                        id>>,
                                                               composite_key_compare<std::less<account_name_type>,
                                                                                     std::
                                                                                         less<change_recovery_account_request_id_type>>>,
                                                ordered_unique<tag<by_effective_date>,
                                                               composite_key<change_recovery_account_request_object,
                                                                             member<change_recovery_account_request_object,
                                                                                    time_point_sec,
                                                                                    &change_recovery_account_request_object::
                                                                                        effective_on>,
                                                                             member<change_recovery_account_request_object,
                                                                                    change_recovery_account_request_id_type,
                                                                                    &change_recovery_account_request_object::
                                                                                        id>>,
                                                               composite_key_compare<std::less<time_point_sec>,
                                                                                     std::
                                                                                         less<change_recovery_account_request_id_type>>>>>
    change_recovery_account_request_index;

typedef shared_multi_index_container<account_registration_bonus_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<account_registration_bonus_object,
                                                                      account_registration_bonus_id_type,
                                                                      &account_registration_bonus_object::id>>,
                                                ordered_unique<tag<by_account>,
                                                               member<account_registration_bonus_object,
                                                                      account_name_type,
                                                                      &account_registration_bonus_object::account>>,
                                                ordered_unique<tag<by_expiration>,
                                                               composite_key<account_registration_bonus_object,
                                                                             member<account_registration_bonus_object,
                                                                                    time_point_sec,
                                                                                    &account_registration_bonus_object::
                                                                                        expires>,
                                                                             member<account_registration_bonus_object,
                                                                                    account_registration_bonus_id_type,
                                                                                    &account_registration_bonus_object::
                                                                                        id>>>>>
    account_registration_bonus_index;

} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT( scorum::chain::account_object,
             (id)(name)(memo_key)(json_metadata)(proxy)(last_account_update)
             (created)(created_by_genesis)
             (owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)(recovery_account)(last_account_recovery)
             (can_vote)(voting_power)(last_vote_time)(voting_power_restoring_time)(vote_reward_competitive_sp)
             (balance)
             (scorumpower)(delegated_scorumpower)(received_scorumpower)
             (proxied_vsf_votes)(witnesses_voted_for)
             (last_post)(last_root_post)
             (active_sp_holders_cashout_time)
             (active_sp_holders_pending_scr_reward)
             (active_sp_holders_pending_sp_reward)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::account_object, scorum::chain::account_index )

FC_REFLECT( scorum::chain::account_blogging_statistic_object,
             (id)(account)
             (post_count)(comment_count)(vote_count)
             (curation_rewards_scr)(curation_rewards_sp)
             (posting_rewards_scr)(posting_rewards_sp)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::account_blogging_statistic_object, scorum::chain::account_blogging_statistic_index )

FC_REFLECT( scorum::chain::account_authority_object,
             (id)(account)(owner)(active)(posting)(last_owner_update)
)
CHAINBASE_SET_INDEX_TYPE( scorum::chain::account_authority_object, scorum::chain::account_authority_index )

FC_REFLECT( scorum::chain::scorumpower_delegation_object,
            (id)(delegator)(delegatee)(scorumpower)(min_delegation_time) )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::scorumpower_delegation_object, scorum::chain::scorumpower_delegation_index )

FC_REFLECT( scorum::chain::reg_pool_sp_delegation_object,
            (id)(delegatee)(sp) )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::reg_pool_sp_delegation_object, scorum::chain::reg_pool_sp_delegation_index )

FC_REFLECT( scorum::chain::scorumpower_delegation_expiration_object,
            (id)(delegator)(scorumpower)(expiration) )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::scorumpower_delegation_expiration_object, scorum::chain::scorumpower_delegation_expiration_index )

FC_REFLECT( scorum::chain::owner_authority_history_object,
             (id)(account)(previous_owner_authority)(last_valid_time)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::owner_authority_history_object, scorum::chain::owner_authority_history_index )

FC_REFLECT( scorum::chain::account_recovery_request_object,
             (id)(account_to_recover)(new_owner_authority)(expires)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::account_recovery_request_object, scorum::chain::account_recovery_request_index )

FC_REFLECT( scorum::chain::change_recovery_account_request_object,
             (id)(account_to_recover)(recovery_account)(effective_on)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::change_recovery_account_request_object, scorum::chain::change_recovery_account_request_index )

FC_REFLECT( scorum::chain::account_registration_bonus_object,
             (id)(account)(bonus)(expires)
          )
CHAINBASE_SET_INDEX_TYPE( scorum::chain::account_registration_bonus_object, scorum::chain::account_registration_bonus_index )

// clang-format on
