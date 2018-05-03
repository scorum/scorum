#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/comment_objects.hpp>
#include <boost/multi_index/composite_key.hpp>

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

dbs_comment::comment_refs_type dbs_comment::get_by_cashout_time(const fc::time_point_sec& upper_bound) const
{
    const auto& idx = db_impl().get_index<comment_index>().indices().get<by_cashout_time>();

    using composite_key_t = std::decay<decltype(idx)>::type::key_from_value;
    using key_t = composite_key_t::result_type;

    struct comp_cashout_time
    {
        bool operator()(fc::time_point_sec t, const key_t& k) const
        {
            return t < k.value.cashout_time;
        }
        bool operator()(const key_t& k, fc::time_point_sec t) const
        {
            return t > k.value.cashout_time;
        }
    };

    auto upper_bound_it = idx.upper_bound(upper_bound, comp_cashout_time{});

    comment_refs_type ret(idx.cbegin(), upper_bound_it);

    return ret;
}

bool dbs_comment::is_exists(const account_name_type& author, const std::string& permlink) const
{
    return find_by<by_permlink>(std::make_tuple(author, permlink)) != nullptr;
}

} // namespace chain
} // namespace scorum
