#pragma once
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/block_summary_object.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/history_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/transaction_object.hpp>
#include <scorum/chain/schema/witness_objects.hpp>
#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/proposal_object.hpp>
#include <scorum/chain/schema/atomicswap_objects.hpp>
#include <scorum/protocol/transaction.hpp>
#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/tags/tags_plugin.hpp>

#include <scorum/witness/witness_objects.hpp>

namespace scorum {
namespace app {

using namespace scorum::chain;

typedef chain::change_recovery_account_request_object change_recovery_account_request_api_obj;
typedef chain::block_summary_object block_summary_api_obj;
typedef chain::comment_vote_object comment_vote_api_obj;
typedef chain::escrow_object escrow_api_obj;
typedef chain::withdraw_vesting_route_object withdraw_vesting_route_api_obj;
typedef chain::decline_voting_rights_request_object decline_voting_rights_request_api_obj;
typedef chain::witness_vote_object witness_vote_api_obj;
typedef chain::witness_schedule_object witness_schedule_api_obj;
typedef chain::vesting_delegation_object vesting_delegation_api_obj;
typedef chain::vesting_delegation_expiration_object vesting_delegation_expiration_api_obj;
typedef chain::reward_fund_object reward_fund_api_obj;
typedef witness::account_bandwidth_object account_bandwidth_api_obj;

struct comment_api_obj
{
    comment_api_obj(const chain::comment_object& o)
        : id(o.id)
        , category(fc::to_string(o.category))
        , parent_author(o.parent_author)
        , parent_permlink(fc::to_string(o.parent_permlink))
        , author(o.author)
        , permlink(fc::to_string(o.permlink))
        , title(fc::to_string(o.title))
        , body(fc::to_string(o.body))
        , json_metadata(fc::to_string(o.json_metadata))
        , last_update(o.last_update)
        , created(o.created)
        , active(o.active)
        , last_payout(o.last_payout)
        , depth(o.depth)
        , children(o.children)
        , net_rshares(o.net_rshares)
        , abs_rshares(o.abs_rshares)
        , vote_rshares(o.vote_rshares)
        , children_abs_rshares(o.children_abs_rshares)
        , cashout_time(o.cashout_time)
        , max_cashout_time(o.max_cashout_time)
        , total_vote_weight(o.total_vote_weight)
        , reward_weight(o.reward_weight)
        , total_payout_value(o.total_payout_value)
        , curator_payout_value(o.curator_payout_value)
        , author_rewards(o.author_rewards)
        , net_votes(o.net_votes)
        , root_comment(o.root_comment)
        , max_accepted_payout(o.max_accepted_payout)
        , percent_scrs(o.percent_scrs)
        , allow_replies(o.allow_replies)
        , allow_votes(o.allow_votes)
        , allow_curation_rewards(o.allow_curation_rewards)
    {
        for (auto& route : o.beneficiaries)
        {
            beneficiaries.push_back(route);
        }
    }

    comment_api_obj()
    {
    }

    comment_id_type id;
    std::string category;
    account_name_type parent_author;
    std::string parent_permlink;
    account_name_type author;
    std::string permlink;

    std::string title;
    std::string body;
    std::string json_metadata;
    time_point_sec last_update;
    time_point_sec created;
    time_point_sec active;
    time_point_sec last_payout;

    uint8_t depth = 0;
    uint32_t children = 0;

    share_type net_rshares;
    share_type abs_rshares;
    share_type vote_rshares;

    share_type children_abs_rshares;
    time_point_sec cashout_time;
    time_point_sec max_cashout_time;
    uint64_t total_vote_weight = 0;

    uint16_t reward_weight = 0;

    asset total_payout_value = asset(0, SCORUM_SYMBOL);
    asset curator_payout_value = asset(0, SCORUM_SYMBOL);

    share_type author_rewards;

    int32_t net_votes = 0;

    comment_id_type root_comment;

    asset max_accepted_payout = asset(0, SCORUM_SYMBOL);
    uint16_t percent_scrs = 0;
    bool allow_replies = false;
    bool allow_votes = false;
    bool allow_curation_rewards = false;
    std::vector<beneficiary_route_type> beneficiaries;
};

struct tag_api_obj
{
    tag_api_obj(const tags::tag_stats_object& o)
        : name(o.tag)
        , total_payouts(o.total_payout)
        , net_votes(o.net_votes)
        , top_posts(o.top_posts)
        , comments(o.comments)
        , trending(o.total_trending)
    {
    }

    tag_api_obj()
    {
    }

    std::string name;
    asset total_payouts = asset(0, SCORUM_SYMBOL);
    int32_t net_votes = 0;
    uint32_t top_posts = 0;
    uint32_t comments = 0;
    fc::uint128 trending = 0;
};

struct account_api_obj
{
    account_api_obj(const chain::account_object& a, const chain::database& db)
        : id(a.id)
        , name(a.name)
        , memo_key(a.memo_key)
        , json_metadata(fc::to_string(a.json_metadata))
        , proxy(a.proxy)
        , last_account_update(a.last_account_update)
        , created(a.created)
        , created_by_genesis(a.created_by_genesis)
        , owner_challenged(a.owner_challenged)
        , active_challenged(a.active_challenged)
        , last_owner_proved(a.last_owner_proved)
        , last_active_proved(a.last_active_proved)
        , recovery_account(a.recovery_account)
        , last_account_recovery(a.last_account_recovery)
        , comment_count(a.comment_count)
        , lifetime_vote_count(a.lifetime_vote_count)
        , post_count(a.post_count)
        , can_vote(a.can_vote)
        , voting_power(a.voting_power)
        , last_vote_time(a.last_vote_time)
        , balance(a.balance)
        , curation_rewards(a.curation_rewards)
        , posting_rewards(a.posting_rewards)
        , vesting_shares(a.vesting_shares)
        , delegated_vesting_shares(a.delegated_vesting_shares)
        , received_vesting_shares(a.received_vesting_shares)
        , vesting_withdraw_rate(a.vesting_withdraw_rate)
        , next_vesting_withdrawal(a.next_vesting_withdrawal)
        , withdrawn(a.withdrawn)
        , to_withdraw(a.to_withdraw)
        , withdraw_routes(a.withdraw_routes)
        , witnesses_voted_for(a.witnesses_voted_for)
        , last_post(a.last_post)
        , last_root_post(a.last_root_post)
    {
        size_t n = a.proxied_vsf_votes.size();
        proxied_vsf_votes.reserve(n);
        for (size_t i = 0; i < n; i++)
            proxied_vsf_votes.push_back(a.proxied_vsf_votes[i]);

        const auto& auth = db.get<account_authority_object, by_account>(name);
        owner = authority(auth.owner);
        active = authority(auth.active);
        posting = authority(auth.posting);
        last_owner_update = auth.last_owner_update;

        if (db.has_index<witness::account_bandwidth_index>())
        {
            auto forum_bandwidth = db.find<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                boost::make_tuple(name, witness::bandwidth_type::forum));

            if (forum_bandwidth != nullptr)
            {
                average_bandwidth = forum_bandwidth->average_bandwidth;
                lifetime_bandwidth = forum_bandwidth->lifetime_bandwidth;
                last_bandwidth_update = forum_bandwidth->last_bandwidth_update;
            }

            auto market_bandwidth = db.find<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                boost::make_tuple(name, witness::bandwidth_type::market));

            if (market_bandwidth != nullptr)
            {
                average_market_bandwidth = market_bandwidth->average_bandwidth;
                lifetime_market_bandwidth = market_bandwidth->lifetime_bandwidth;
                last_market_bandwidth_update = market_bandwidth->last_bandwidth_update;
            }
        }
    }

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
    uint32_t comment_count = 0;
    uint32_t lifetime_vote_count = 0;
    uint32_t post_count = 0;

    bool can_vote = false;
    uint16_t voting_power = 0;
    time_point_sec last_vote_time;

    asset balance = asset(0, SCORUM_SYMBOL);

    share_type curation_rewards;
    share_type posting_rewards;

    asset vesting_shares = asset(0, VESTS_SYMBOL);
    asset delegated_vesting_shares = asset(0, VESTS_SYMBOL);
    asset received_vesting_shares = asset(0, VESTS_SYMBOL);
    asset vesting_withdraw_rate = asset(0, VESTS_SYMBOL);
    time_point_sec next_vesting_withdrawal;
    share_type withdrawn;
    share_type to_withdraw;
    uint16_t withdraw_routes = 0;

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
};

struct account_balance_info_api_obj
{
    account_balance_info_api_obj(const account_api_obj& a)
        : balance(a.balance)
        , vesting_shares(a.vesting_shares)
    {
    }

    account_balance_info_api_obj()
    {
    }

    asset balance = asset(0, SCORUM_SYMBOL);
    asset vesting_shares = asset(0, VESTS_SYMBOL);
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

struct account_history_api_obj
{
};

struct proposal_api_obj
{
    proposal_api_obj(const proposal_object& p)
        : id(p.id)
        , creator(p.creator)
        , data(p.data)
        , expiration(p.expiration)
        , quorum_percent(p.quorum_percent)
        , action(p.action)
        , voted_accounts(p.voted_accounts)
    {
    }

    proposal_api_obj()
    {
    }

    proposal_object::id_type id;

    account_name_type creator;
    fc::variant data;

    fc::time_point_sec expiration;

    uint64_t quorum_percent = 0;

    scorum::protocol::proposal_action action = scorum::protocol::proposal_action::invite;
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
        , props(w.props)
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
    chain_properties props;
    share_type votes;
    fc::uint128 virtual_last_update;
    fc::uint128 virtual_position;
    fc::uint128 virtual_scheduled_time;
    version running_version;
    hardfork_version hardfork_version_vote;
    time_point_sec hardfork_time_vote;
};

struct signed_block_api_obj : public signed_block
{
    signed_block_api_obj(const signed_block& block)
        : signed_block(block)
    {
        block_id = id();
        signing_key = signee();
        transaction_ids.reserve(transactions.size());
        for (const signed_transaction& tx : transactions)
            transaction_ids.push_back(tx.id());
    }
    signed_block_api_obj()
    {
    }

    block_id_type block_id;
    public_key_type signing_key;
    std::vector<transaction_id_type> transaction_ids;
};

struct dynamic_global_property_api_obj : public dynamic_global_property_object
{
    dynamic_global_property_api_obj(const dynamic_global_property_object& gpo, const chain::database& db)
        : dynamic_global_property_object(gpo)
    {
        if (db.has_index<witness::reserve_ratio_index>())
        {
            const auto& r = db.find(witness::reserve_ratio_id_type());

            if (BOOST_LIKELY(r != nullptr))
            {
                current_reserve_ratio = r->current_reserve_ratio;
                average_block_size = r->average_block_size;
                max_virtual_bandwidth = r->max_virtual_bandwidth;
            }
        }
    }

    dynamic_global_property_api_obj(const dynamic_global_property_object& gpo)
        : dynamic_global_property_object(gpo)
    {
    }

    dynamic_global_property_api_obj()
    {
    }

    uint32_t current_reserve_ratio = 0;
    uint64_t average_block_size = 0;
    uint128_t max_virtual_bandwidth = 0;
};

struct budget_api_obj
{
    budget_api_obj(const chain::budget_object& b)
        : id(b.id._id)
        , owner(b.owner)
        , content_permlink(fc::to_string(b.content_permlink))
        , created(b.created)
        , deadline(b.deadline)
        , balance(b.balance)
        , per_block(b.per_block)
        , last_allocated_block(b.last_allocated_block)
    {
    }

    // because fc::variant require for temporary object
    budget_api_obj()
    {
    }

    int64_t id;

    account_name_type owner;
    std::string content_permlink;

    time_point_sec created;
    time_point_sec deadline;

    asset balance = asset(0, SCORUM_SYMBOL);
    share_type per_block;

    uint32_t last_allocated_block;
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
        obj.contract_initiator = (op.type == protocol::atomicswap_by_initiator);
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

} // namespace app
} // namespace scorum

// clang-format off

FC_REFLECT( scorum::app::comment_api_obj,
             (id)(author)(permlink)
             (category)(parent_author)(parent_permlink)
             (title)(body)(json_metadata)(last_update)(created)(active)(last_payout)
             (depth)(children)
             (net_rshares)(abs_rshares)(vote_rshares)
             (children_abs_rshares)(cashout_time)(max_cashout_time)
             (total_vote_weight)(reward_weight)(total_payout_value)(curator_payout_value)(author_rewards)(net_votes)(root_comment)
             (max_accepted_payout)(percent_scrs)(allow_replies)(allow_votes)(allow_curation_rewards)
             (beneficiaries)
          )

FC_REFLECT( scorum::app::account_api_obj,
             (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)(created_by_genesis)
             (owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)(recovery_account)(last_account_recovery)
             (comment_count)(lifetime_vote_count)(post_count)(can_vote)(voting_power)(last_vote_time)
             (balance)
             (vesting_shares)(delegated_vesting_shares)(received_vesting_shares)(vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
             (curation_rewards)
             (posting_rewards)
             (proxied_vsf_votes)(witnesses_voted_for)
             (average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update)
             (average_market_bandwidth)(lifetime_market_bandwidth)(last_market_bandwidth_update)
             (last_post)(last_root_post)
          )

FC_REFLECT (scorum::app::account_balance_info_api_obj,
            (balance)(vesting_shares))

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

FC_REFLECT( scorum::app::tag_api_obj,
            (name)
            (total_payouts)
            (net_votes)
            (top_posts)
            (comments)
            (trending)
          )

FC_REFLECT( scorum::app::witness_api_obj,
             (id)
             (owner)
             (created)
             (url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_confirmed_block_num)(signing_key)
             (props)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
          )

FC_REFLECT( scorum::app::proposal_api_obj,
            (id)
            (creator)
            (expiration)
            (voted_accounts)
            (quorum_percent)
            (action)
            (data)
          )

FC_REFLECT_DERIVED( scorum::app::signed_block_api_obj, (scorum::protocol::signed_block),
                     (block_id)
                     (signing_key)
                     (transaction_ids)
                  )

FC_REFLECT_DERIVED( scorum::app::dynamic_global_property_api_obj, (scorum::chain::dynamic_global_property_object),
                     (current_reserve_ratio)
                     (average_block_size)
                     (max_virtual_bandwidth)
                  )

FC_REFLECT( scorum::app::budget_api_obj,
            (id)
            (owner)
            (content_permlink)
            (created)
            (deadline)
            (balance)
            (per_block)
            (last_allocated_block)
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
