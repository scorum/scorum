#include <scorum/chain/services/account_blogging_statistic.hpp>

namespace scorum {
namespace chain {

dbs_account_blogging_statistic::dbs_account_blogging_statistic(database& db)
    : base_service_type(db)
{
}

const account_blogging_statistic_object& dbs_account_blogging_statistic::obtain(const account_id_type& account_id)
{
    try
    {
        if (nullptr == find_by<by_account_id>(account_id))
        {
            return create([&](account_blogging_statistic_object& stat) { stat.account = account_id; });
        }
        else
        {
            return get_by<by_account_id>(account_id);
        }
    }
    FC_CAPTURE_AND_RETHROW((account_id))
}

const account_blogging_statistic_object* dbs_account_blogging_statistic::find(const account_id_type& account_id) const
{
    return find_by<by_account_id>(account_id);
}

void dbs_account_blogging_statistic::add_post(const account_blogging_statistic_object& stat)
{
    update(stat, [&](account_blogging_statistic_object& a) { a.post_count++; });
}

void dbs_account_blogging_statistic::add_comment(const account_blogging_statistic_object& stat)
{
    update(stat, [&](account_blogging_statistic_object& a) { a.comment_count++; });
}

void dbs_account_blogging_statistic::add_vote(const account_blogging_statistic_object& stat)
{
    update(stat, [&](account_blogging_statistic_object& a) { a.vote_count++; });
}

void dbs_account_blogging_statistic::increase_posting_rewards(const account_blogging_statistic_object& stat,
                                                              const asset& reward)
{
    if (SCORUM_SYMBOL == reward.symbol())
    {
        update(stat, [&](account_blogging_statistic_object& a) { a.posting_rewards_scr += reward; });
    }
    else if (SP_SYMBOL == reward.symbol())
    {
        update(stat, [&](account_blogging_statistic_object& a) { a.posting_rewards_sp += reward; });
    }
}

void dbs_account_blogging_statistic::increase_curation_rewards(const account_blogging_statistic_object& stat,
                                                               const asset& reward)
{
    if (SCORUM_SYMBOL == reward.symbol())
    {
        update(stat, [&](account_blogging_statistic_object& a) { a.curation_rewards_scr += reward; });
    }
    else if (SP_SYMBOL == reward.symbol())
    {
        update(stat, [&](account_blogging_statistic_object& a) { a.curation_rewards_sp += reward; });
    }
}
}
}
