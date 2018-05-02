#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/database/database.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

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

dbs_comment::comment_refs_type dbs_comment::get_by_cashout_time(const fc::time_point_sec& until) const
{
    try
    {
        return get_range_by<by_cashout_time>(::boost::multi_index::unbounded,
                                             ::boost::lambda::_1 <= std::make_tuple(until, ALL_IDS));
    }
    FC_CAPTURE_AND_RETHROW((until))
}

dbs_comment::comment_refs_type dbs_comment::get_by_create_time(const fc::time_point_sec& until,
                                                               const checker_type& filter) const
{
    try
    {
        return get_filtered_range_by<by_created>(::boost::multi_index::unbounded,
                                                 ::boost::lambda::_1 <= std::make_tuple(until, ALL_IDS), filter);
    }
    FC_CAPTURE_AND_RETHROW((until))
}

bool dbs_comment::is_exists(const account_name_type& author, const std::string& permlink) const
{
    return find_by<by_permlink>(std::make_tuple(author, permlink)) != nullptr;
}

} // namespace chain
} // namespace scorum
