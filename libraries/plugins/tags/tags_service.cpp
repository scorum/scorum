#include <scorum/tags/tags_service.hpp>

#include <scorum/tags/tags_api_objects.hpp>

#include <scorum/chain/database/database.hpp>

namespace scorum {
namespace tags {

tags_service::tags_service(scorum::chain::database& db)
    : _base_type(db)
{
}

void tags_service::set_promoted_balance(chain::comment_id_type id, asset& promoted) const
{
    const auto& cidx = db_impl().get_index<tags::tag_index>().indices().get<tags::by_comment>();
    auto itr = cidx.lower_bound(id);
    if (itr != cidx.end() && itr->comment == id)
    {
        promoted = asset(itr->promoted_balance, SCORUM_SYMBOL);
    }
}

const fc::time_point_sec tags_service::calculate_discussion_payout_time(const comment_object& comment) const
{
    return comment.cashout_time;
}

} // namespace tags
} // namespace scorum
