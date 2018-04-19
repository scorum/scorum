#include <scorum/app/scorum_api_objects.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>

#include <scorum/chain/database/database.hpp>

namespace scorum {
namespace app {

comment_api_obj::comment_api_obj(const chain::comment_object& o)
{
    set_comment(o);
    initialize(o);
}

comment_api_obj::comment_api_obj(const chain::comment_object& o, const chain::database& db)
{
    set_comment(o);
    dbs_comment_statistic& comment_statistic_service = db.obtain_service<dbs_comment_statistic>();
    set_comment_statistic(comment_statistic_service.get(o.id));
    initialize(o);
}

void comment_api_obj::set_comment(const chain::comment_object& o)
{
    id = o.id;
    category = fc::to_string(o.category);
    parent_author = o.parent_author;
    parent_permlink = fc::to_string(o.parent_permlink);
    author = o.author;
    permlink = fc::to_string(o.permlink);
    title = fc::to_string(o.title);
    body = fc::to_string(o.body);
    json_metadata = fc::to_string(o.json_metadata);
    last_update = o.last_update;
    created = o.created;
    active = o.active;
    last_payout = o.last_payout;
    depth = o.depth;
    children = o.children;
    net_rshares = o.net_rshares;
    abs_rshares = o.abs_rshares;
    vote_rshares = o.vote_rshares;
    children_abs_rshares = o.children_abs_rshares;
    cashout_time = o.cashout_time;
    max_cashout_time = o.max_cashout_time;
    total_vote_weight = o.total_vote_weight;
    net_votes = o.net_votes;
    root_comment = o.root_comment;
    max_accepted_payout = o.max_accepted_payout;
    allow_replies = o.allow_replies;
    allow_votes = o.allow_votes;
    allow_curation_rewards = o.allow_curation_rewards;
}

void comment_api_obj::set_comment_statistic(const chain::comment_statistic_object& stat)
{
    total_payout_scr_value = stat.total_payout_scr_value;
    total_payout_sp_value = stat.total_payout_sp_value;
    author_payout_scr_value = stat.author_payout_scr_value;
    author_payout_sp_value = stat.author_payout_sp_value;
    curator_payout_scr_value = stat.curator_payout_scr_value;
    curator_payout_sp_value = stat.curator_payout_sp_value;
    beneficiary_payout_scr_value = stat.beneficiary_payout_scr_value;
    beneficiary_payout_sp_value = stat.beneficiary_payout_sp_value;
}

void comment_api_obj::initialize(const chain::comment_object& o)
{
    for (auto& route : o.beneficiaries)
    {
        beneficiaries.push_back(route);
    }
}

account_api_obj::account_api_obj(const chain::account_object& a, const chain::database& db)
{
    set_account(a);
    dbs_account_blogging_statistic& account_blogging_statistic_service
        = db.obtain_service<dbs_account_blogging_statistic>();
    const auto* pstat = account_blogging_statistic_service.find(a.id);
    if (pstat)
    {
        set_account_blogging_statistic(*pstat);
    }
    initialize(a, db);
}

void account_api_obj::set_account(const chain::account_object& a)
{
    id = a.id;
    name = a.name;
    memo_key = a.memo_key;
    json_metadata = fc::to_string(a.json_metadata);
    proxy = a.proxy;
    last_account_update = a.last_account_update;
    created = a.created;
    created_by_genesis = a.created_by_genesis;
    owner_challenged = a.owner_challenged;
    active_challenged = a.active_challenged;
    last_owner_proved = a.last_owner_proved;
    last_active_proved = a.last_active_proved;
    recovery_account = a.recovery_account;
    last_account_recovery = a.last_account_recovery;
    can_vote = a.can_vote;
    voting_power = a.voting_power;
    last_vote_time = a.last_vote_time;
    balance = a.balance;
    scorumpower = a.scorumpower;
    delegated_scorumpower = a.delegated_scorumpower;
    received_scorumpower = a.received_scorumpower;
    witnesses_voted_for = a.witnesses_voted_for;
    last_post = a.last_post;
    last_root_post = a.last_root_post;
}

void account_api_obj::set_account_blogging_statistic(const chain::account_blogging_statistic_object& s)
{
    post_count = s.post_count;
    comment_count = s.comment_count;
    vote_count = s.vote_count;
    curation_rewards_scr = s.curation_rewards_scr;
    curation_rewards_sp = s.curation_rewards_sp;
    posting_rewards_scr = s.posting_rewards_scr;
    posting_rewards_sp = s.posting_rewards_sp;
}

void account_api_obj::initialize(const chain::account_object& a, const chain::database& db)
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
}
}
