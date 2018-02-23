#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/comment_objects.hpp>

#include <tuple>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_comment::dbs_comment(database& db)
    : _base_type(db)
{
}

const comment_object& dbs_comment::get(const comment_id_type& comment_id) const
{
    try
    {
        return db_impl().get(comment_id);
    }
    FC_CAPTURE_AND_RETHROW((comment_id))
}

const comment_object& dbs_comment::get(const account_name_type& author, const std::string& permlink) const
{
    try
    {
        return db_impl().get<comment_object, by_permlink>(boost::make_tuple(author, permlink));
    }
    FC_CAPTURE_AND_RETHROW((author)(permlink))
}

dbs_comment::comment_refs_type dbs_comment::get_by_cashout_time() const
{
    comment_refs_type ret;

    const auto& idx = db_impl().get_index<comment_index>().indices().get<by_cashout_time>();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

bool dbs_comment::is_exists(const account_name_type& author, const std::string& permlink) const
{
    return nullptr != db_impl().find<comment_object, by_permlink>(std::make_tuple(author, permlink));
}

const comment_object& dbs_comment::create(const modifier_type& modifier)
{
    const auto& new_comment = db_impl().create<comment_object>([&](comment_object& c) { modifier(c); });

    return new_comment;
}

void dbs_comment::update(const comment_object& comment, const modifier_type& modifier)
{
    db_impl().modify(comment, [&](comment_object& c) { modifier(c); });
}

void dbs_comment::remove(const comment_object& comment)
{
    db_impl().remove(comment);
}

} // namespace chain
} // namespace scorum
