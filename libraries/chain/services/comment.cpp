#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/comment_objects.hpp>

#include <tuple>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_comment::dbs_comment(database& db)
    : base_service_type(db)
{
}

const comment_object& dbs_comment::get(const comment_id_type& comment_id) const
{
    try
    {
        return get_by(comment_id);
    }
    FC_CAPTURE_AND_RETHROW((comment_id))
}

const comment_object& dbs_comment::get(const account_name_type& author, const std::string& permlink) const
{
    try
    {
        return get_by<by_permlink>(boost::make_tuple(author, permlink));
    }
    FC_CAPTURE_AND_RETHROW((author)(permlink))
}

dbs_comment::comment_refs_type dbs_comment::get_by_cashout_time(const checker_type& until) const
{
    return get_by_cashout_time(until, [](const comment_object&) { return true; });
}

dbs_comment::comment_refs_type dbs_comment::get_by_cashout_time(const checker_type& until,
                                                                const checker_type& filter) const
{
    comment_refs_type ret;

    const auto& idx = db_impl().get_index<comment_index>().indices().get<by_cashout_time>();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    for (; it != it_end; ++it)
    {
        if (!until(*it))
            break;

        if (!filter(*it))
            continue;

        ret.push_back(std::cref(*it));
    }

    return ret;
}

bool dbs_comment::is_exists(const account_name_type& author, const std::string& permlink) const
{
    return find_by<by_permlink>(std::make_tuple(author, permlink)) != nullptr;
}

dbs_comment_statistic::dbs_comment_statistic(database& db)
    : base_service_type(db)
{
}

const comment_statistic_object& dbs_comment_statistic::get(const comment_id_type& comment_id) const
{
    try
    {
        return get_by<by_comment_id>(comment_id);
    }
    FC_CAPTURE_AND_RETHROW((comment_id))
}

} // namespace chain
} // namespace scorum
