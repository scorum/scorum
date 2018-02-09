#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/comment_objects.hpp>

#include <tuple>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_comment_vote::dbs_comment_vote(database& db)
    : _base_type(db)
{
}

const comment_vote_object& dbs_comment_vote::get(const comment_id_type& comment_id,
                                                 const account_id_type& voter_id) const
{
    try
    {
        return db_impl().get<comment_vote_object, by_comment_voter>(boost::make_tuple(comment_id, voter_id));
    }
    FC_CAPTURE_AND_RETHROW((comment_id)(voter_id))
}

bool dbs_comment_vote::is_exists(const comment_id_type& comment_id, const account_id_type& voter_id) const
{
    return nullptr != db_impl().find<comment_vote_object, by_comment_voter>(std::make_tuple(comment_id, voter_id));
}

const comment_vote_object& dbs_comment_vote::create(const modifier_type& modifier)
{
    const auto& new_comment_vote
        = db_impl().create<comment_vote_object>([&](comment_vote_object& cvo) { modifier(cvo); });

    return new_comment_vote;
}

void dbs_comment_vote::update(const comment_vote_object& comment_vote, const modifier_type& modifier)
{
    db_impl().modify(comment_vote, [&](comment_vote_object& cvo) { modifier(cvo); });
}

void dbs_comment_vote::remove(const comment_id_type& comment_id)
{
    const auto& vote_idx = db_impl().get_index<comment_vote_index>().indices().get<by_comment_voter>();

    auto vote_itr = vote_idx.lower_bound(comment_id_type(comment_id));
    while (vote_itr != vote_idx.end() && vote_itr->comment == comment_id)
    {
        const auto& cur_vote = *vote_itr;
        ++vote_itr;
        db_impl().remove(cur_vote);
    }
}

} // namespace chain
} // namespace scorum
