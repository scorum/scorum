#pragma once
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/block_summary_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/transaction_object.hpp>
#include <scorum/chain/schema/witness_objects.hpp>
#include <scorum/chain/schema/budget_objects.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/proposal_object.hpp>
#include <scorum/chain/schema/atomicswap_objects.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

#include <scorum/protocol/transaction.hpp>
#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/tags/tags_plugin.hpp>

#include <scorum/witness/witness_objects.hpp>

#include <scorum/app/schema/api_template.hpp>

namespace scorum {
namespace app {

using namespace scorum::chain;

struct development_committee_api_obj
{
    development_committee_api_obj(const chain::dev_committee_object& obj);

    development_committee_api_obj()
    {
    }

    asset sp_balance = asset(0, SP_SYMBOL);
    asset scr_balance = asset(0, SCORUM_SYMBOL);

    fc::flat_map<budget_type, uint16_t> top_budgets_amounts;

    protocol::percent_type transfer_quorum = SCORUM_COMMITTEE_TRANSFER_QUORUM_PERCENT;
    protocol::percent_type invite_quorum = SCORUM_COMMITTEE_ADD_EXCLUDE_QUORUM_PERCENT;
    protocol::percent_type dropout_quorum = SCORUM_COMMITTEE_ADD_EXCLUDE_QUORUM_PERCENT;
    protocol::percent_type change_quorum = SCORUM_COMMITTEE_QUORUM_PERCENT;
    protocol::percent_type top_budgets_amounts_quorum = SCORUM_COMMITTEE_QUORUM_PERCENT;
};

typedef api_obj<scorum::chain::block_summary_object> block_summary_api_obj;
typedef api_obj<scorum::chain::change_recovery_account_request_object> change_recovery_account_request_api_obj;
typedef api_obj<scorum::chain::decline_voting_rights_request_object> decline_voting_rights_request_api_obj;
typedef api_obj<scorum::chain::escrow_object> escrow_api_obj;
typedef api_obj<scorum::chain::scorumpower_delegation_expiration_object> scorumpower_delegation_expiration_api_obj;
typedef api_obj<scorum::chain::scorumpower_delegation_object> scorumpower_delegation_api_obj;
typedef api_obj<scorum::chain::withdraw_scorumpower_route_object> withdraw_scorumpower_route_api_obj;
typedef api_obj<scorum::chain::witness_schedule_object> witness_schedule_api_obj;
typedef api_obj<scorum::chain::witness_vote_object> witness_vote_api_obj;
typedef api_obj<scorum::witness::account_bandwidth_object> account_bandwidth_api_obj;
typedef api_obj<scorum::witness::reserve_ratio_object> reserve_ratio_api_obj;

struct dynamic_global_property_api_obj : public api_obj<scorum::chain::dynamic_global_property_object>,
                                         public api_obj<scorum::witness::reserve_ratio_object>
{
    template <class T> dynamic_global_property_api_obj& operator=(const T& other)
    {
        T& base = static_cast<T&>(*this);
        base = other;
        return *this;
    }

    asset registration_pool_balance = asset(0, SCORUM_SYMBOL);
    asset fund_budget_balance = asset(0, SP_SYMBOL);
    asset reward_pool_balance = asset(0, SCORUM_SYMBOL);
    asset content_reward_scr_balance = asset(0, SCORUM_SYMBOL);
    asset content_reward_sp_balance = asset(0, SP_SYMBOL);
};

struct account_api_obj
{
    account_api_obj(const chain::account_object& a, const chain::database& db);

    account_api_obj()
    {
    }

    account_id_type id;

    account_name_type name;
    authority owner;
    authority active;
    authority posting;
    public_key_type memo_key;
    std::string json_metadata;
    account_name_type proxy;

    time_point_sec last_owner_update;
    time_point_sec last_account_update;

    time_point_sec created;
    bool created_by_genesis = false;
    bool owner_challenged = false;
    bool active_challenged = false;
    time_point_sec last_owner_proved;
    time_point_sec last_active_proved;
    account_name_type recovery_account;
    time_point_sec last_account_recovery;

    bool can_vote = false;
    uint16_t voting_power = 0;
    time_point_sec last_vote_time;

    asset balance = asset(0, SCORUM_SYMBOL);

    asset scorumpower = asset(0, SP_SYMBOL);
    asset delegated_scorumpower = asset(0, SP_SYMBOL);
    asset received_scorumpower = asset(0, SP_SYMBOL);

    std::vector<share_type> proxied_vsf_votes;

    uint16_t witnesses_voted_for;

    share_type average_bandwidth = 0;
    share_type lifetime_bandwidth = 0;
    time_point_sec last_bandwidth_update;

    share_type average_market_bandwidth = 0;
    share_type lifetime_market_bandwidth = 0;
    time_point_sec last_market_bandwidth_update;

    time_point_sec last_post;
    time_point_sec last_root_post;

    uint32_t post_count = 0;
    uint32_t comment_count = 0;
    uint32_t vote_count = 0;

    asset curation_rewards_scr = asset(0, SCORUM_SYMBOL);
    asset curation_rewards_sp = asset(0, SP_SYMBOL);
    asset posting_rewards_scr = asset(0, SCORUM_SYMBOL);
    asset posting_rewards_sp = asset(0, SP_SYMBOL);

private:
    inline void set_account(const chain::account_object&);
    inline void set_account_blogging_statistic(const chain::account_blogging_statistic_object&);
    inline void initialize(const chain::account_object& a, const chain::database& db);
};

struct account_balance_info_api_obj
{
    account_balance_info_api_obj(const account_api_obj& a)
        : balance(a.balance)
        , scorumpower(a.scorumpower)
    {
    }

    account_balance_info_api_obj()
    {
    }

    asset balance = asset(0, SCORUM_SYMBOL);
    asset scorumpower = asset(0, SP_SYMBOL);
};

struct owner_authority_history_api_obj
{
    owner_authority_history_api_obj(const chain::owner_authority_history_object& o)
        : id(o.id)
        , account(o.account)
        , previous_owner_authority(authority(o.previous_owner_authority))
        , last_valid_time(o.last_valid_time)
    {
    }

    owner_authority_history_api_obj()
    {
    }

    owner_authority_history_id_type id;

    account_name_type account;
    authority previous_owner_authority;
    time_point_sec last_valid_time;
};

struct account_recovery_request_api_obj
{
    account_recovery_request_api_obj(const chain::account_recovery_request_object& o)
        : id(o.id)
        , account_to_recover(o.account_to_recover)
        , new_owner_authority(authority(o.new_owner_authority))
        , expires(o.expires)
    {
    }

    account_recovery_request_api_obj()
    {
    }

    account_recovery_request_id_type id;
    account_name_type account_to_recover;
    authority new_owner_authority;
    time_point_sec expires;
};

struct proposal_api_obj
{
    proposal_api_obj(const proposal_object& p)
        : id(p.id)
        , operation(p.operation)
        , creator(p.creator)
        , expiration(p.expiration)
        , quorum_percent(p.quorum_percent)
    {
        for (auto& a : p.voted_accounts)
        {
            voted_accounts.insert(a);
        }
    }

    proposal_api_obj()
    {
    }

    proposal_object::id_type id;

    protocol::proposal_operation operation;

    account_name_type creator;

    fc::time_point_sec expiration;

    uint64_t quorum_percent = 0;

    flat_set<account_name_type> voted_accounts;
};

struct witness_api_obj
{
    witness_api_obj(const chain::witness_object& w)
        : id(w.id)
        , owner(w.owner)
        , created(w.created)
        , url(fc::to_string(w.url))
        , total_missed(w.total_missed)
        , last_confirmed_block_num(w.last_confirmed_block_num)
        , signing_key(w.signing_key)
        , proposed_chain_props(w.proposed_chain_props)
        , votes(w.votes)
        , virtual_last_update(w.virtual_last_update)
        , virtual_position(w.virtual_position)
        , virtual_scheduled_time(w.virtual_scheduled_time)
        , running_version(w.running_version)
        , hardfork_version_vote(w.hardfork_version_vote)
        , hardfork_time_vote(w.hardfork_time_vote)
    {
    }

    witness_api_obj()
    {
    }

    witness_id_type id;
    account_name_type owner;
    time_point_sec created;
    std::string url;
    uint32_t total_missed = 0;
    uint64_t last_confirmed_block_num = 0;
    public_key_type signing_key;
    chain_properties proposed_chain_props;
    share_type votes;
    fc::uint128 virtual_last_update;
    fc::uint128 virtual_position;
    fc::uint128 virtual_scheduled_time;
    version running_version;
    hardfork_version hardfork_version_vote;
    time_point_sec hardfork_time_vote;
};

class budget_api_obj
{
public:
    budget_api_obj(const chain::post_budget_object& b)
        : id(b.id._id)
        , type(budget_type::post)
        , owner(b.owner)
        , content_permlink(fc::to_string(b.content_permlink))
        , created(b.created)
        , start(b.start)
        , deadline(b.deadline)
        , balance(b.balance)
        , per_block(b.per_block)
        , last_cashout_block(b.last_cashout_block)
    {
    }

    budget_api_obj(const chain::banner_budget_object& b)
        : id(b.id._id)
        , type(budget_type::banner)
        , owner(b.owner)
        , content_permlink(fc::to_string(b.content_permlink))
        , created(b.created)
        , start(b.start)
        , deadline(b.deadline)
        , balance(b.balance)
        , per_block(b.per_block)
        , last_cashout_block(b.last_cashout_block)
    {
    }

    // because fc::variant require for temporary object
    budget_api_obj()
    {
    }

    int64_t id;

    budget_type type;

    account_name_type owner;
    std::string content_permlink;

    time_point_sec created;
    time_point_sec start;
    time_point_sec deadline;

    asset balance = asset(0, SCORUM_SYMBOL);
    asset per_block;

    uint32_t last_cashout_block = 0;
};

struct atomicswap_contract_api_obj
{
    atomicswap_contract_api_obj(const chain::atomicswap_contract_object& c)
        : id(c.id._id)
        , contract_initiator(c.type == chain::atomicswap_contract_initiator)
        , owner(c.owner)
        , to(c.to)
        , amount(c.amount)
        , created(c.created)
        , deadline(c.deadline)
        , metadata(fc::to_string(c.metadata))
    {
    }

    // because fc::variant require for temporary object
    atomicswap_contract_api_obj()
    {
    }

    int64_t id;

    bool contract_initiator;

    account_name_type owner;

    account_name_type to;
    asset amount = asset(0, SCORUM_SYMBOL);

    time_point_sec created;
    time_point_sec deadline;

    std::string metadata;
};

struct atomicswap_contract_info_api_obj : public atomicswap_contract_api_obj
{
    atomicswap_contract_info_api_obj(const chain::atomicswap_contract_object& c)
        : atomicswap_contract_api_obj(c)
        , secret(fc::to_string(c.secret))
        , secret_hash(fc::to_string(c.secret_hash))
    {
    }

    // because fc::variant require for temporary object
    atomicswap_contract_info_api_obj()
    {
    }

    std::string secret;
    std::string secret_hash;

    bool empty() const
    {
        return secret_hash.empty() || !owner.size() || !to.size() || amount.amount == 0;
    }
};

struct atomicswap_contract_result_api_obj
{
    atomicswap_contract_result_api_obj(const protocol::annotated_signed_transaction& _tr)
        : tr(_tr)
    {
    }

    atomicswap_contract_result_api_obj(const protocol::annotated_signed_transaction& _tr,
                                       const protocol::atomicswap_initiate_operation& op,
                                       const std::string& secret = "")
        : tr(_tr)
    {
        obj.contract_initiator = (op.type == protocol::atomicswap_initiate_operation::by_initiator);
        obj.owner = op.owner;
        obj.to = op.recipient;
        obj.amount = op.amount;
        obj.metadata = op.metadata;
        obj.secret_hash = op.secret_hash;
        obj.secret = secret;
    }

    atomicswap_contract_result_api_obj()
    {
    }

    protocol::annotated_signed_transaction tr;
    atomicswap_contract_info_api_obj obj;

    bool empty() const
    {
        return obj.empty();
    }
};

struct registration_committee_api_obj
{
    registration_committee_api_obj()
    {
    }

    registration_committee_api_obj(const registration_pool_object& reg_committee)
        : invite_quorum(reg_committee.invite_quorum)
        , dropout_quorum(reg_committee.dropout_quorum)
        , change_quorum(reg_committee.change_quorum)
    {
    }

    protocol::percent_type invite_quorum = 0u;
    protocol::percent_type dropout_quorum = 0u;
    protocol::percent_type change_quorum = 0u;
};

} // namespace app
} // namespace scorum

// clang-format off

FC_REFLECT_DERIVED(scorum::app::account_bandwidth_api_obj, (scorum::witness::account_bandwidth_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::block_summary_api_obj, (scorum::chain::block_summary_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::change_recovery_account_request_api_obj, (scorum::chain::change_recovery_account_request_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::decline_voting_rights_request_api_obj, (scorum::chain::decline_voting_rights_request_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::escrow_api_obj, (scorum::chain::escrow_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::scorumpower_delegation_api_obj, (scorum::chain::scorumpower_delegation_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::scorumpower_delegation_expiration_api_obj, (scorum::chain::scorumpower_delegation_expiration_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::withdraw_scorumpower_route_api_obj, (scorum::chain::withdraw_scorumpower_route_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::witness_schedule_api_obj, (scorum::chain::witness_schedule_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::witness_vote_api_obj, (scorum::chain::witness_vote_object), BOOST_PP_SEQ_NIL)

FC_REFLECT_DERIVED(scorum::app::dynamic_global_property_api_obj,
                   (scorum::chain::dynamic_global_property_object)(scorum::witness::reserve_ratio_object),
                   (registration_pool_balance)
                   (fund_budget_balance)
                   (reward_pool_balance)
                   (content_reward_scr_balance)
                   (content_reward_sp_balance)
                   )
FC_REFLECT(scorum::app::development_committee_api_obj,
           (sp_balance)
           (scr_balance)
           (top_budgets_amounts)
           (transfer_quorum)
           (invite_quorum)
           (dropout_quorum)
           (change_quorum)
           (top_budgets_amounts_quorum)
           )
FC_REFLECT(scorum::app::registration_committee_api_obj, (invite_quorum)(dropout_quorum)(change_quorum))

FC_REFLECT( scorum::app::account_api_obj,
             (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)(created_by_genesis)
             (owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)(recovery_account)(last_account_recovery)
             (can_vote)(voting_power)(last_vote_time)
             (balance)
             (scorumpower)(delegated_scorumpower)(received_scorumpower)
             (proxied_vsf_votes)(witnesses_voted_for)
             (average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update)
             (average_market_bandwidth)(lifetime_market_bandwidth)(last_market_bandwidth_update)
             (last_post)(last_root_post)
             (post_count)(comment_count)(vote_count)
             (curation_rewards_scr)
             (curation_rewards_sp)
             (posting_rewards_scr)
             (posting_rewards_sp)
          )

FC_REFLECT (scorum::app::account_balance_info_api_obj,
            (balance)(scorumpower))

FC_REFLECT( scorum::app::owner_authority_history_api_obj,
             (id)
             (account)
             (previous_owner_authority)
             (last_valid_time)
          )

FC_REFLECT( scorum::app::account_recovery_request_api_obj,
             (id)
             (account_to_recover)
             (new_owner_authority)
             (expires)
          )

FC_REFLECT( scorum::app::witness_api_obj,
             (id)
             (owner)
             (created)
             (url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_confirmed_block_num)(signing_key)
             (proposed_chain_props)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
          )

FC_REFLECT( scorum::app::proposal_api_obj,
            (id)
            (creator)
            (expiration)
            (operation)
            (voted_accounts)
            (quorum_percent)
          )

FC_REFLECT( scorum::app::budget_api_obj,
            (id)
            (type)
            (owner)
            (content_permlink)
            (created)
            (start)
            (deadline)
            (balance)
            (per_block)
            (last_cashout_block)
          )

FC_REFLECT( scorum::app::atomicswap_contract_api_obj,
            (id)
            (contract_initiator)
            (owner)
            (to)
            (amount)
            (created)
            (deadline)
            (metadata)
          )

FC_REFLECT_DERIVED( scorum::app::atomicswap_contract_info_api_obj, (scorum::app::atomicswap_contract_api_obj),
                     (secret)
                     (secret_hash)
                  )

FC_REFLECT( scorum::app::atomicswap_contract_result_api_obj,
            (tr)
            (obj)
          )

// clang-format on
