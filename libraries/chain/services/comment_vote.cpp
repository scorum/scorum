#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/comment_objects.hpp>

#include <tuple>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_comment_vote::dbs_comment_vote(database& db)
    : base_service_type(db)
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

dbs_comment_vote::comment_vote_refs_type dbs_comment_vote::get_by_comment(const comment_id_type& comment_id) const
{
    comment_vote_refs_type ret;

    const auto& vote_idx = db_impl().get_index<comment_vote_index>().indices().get<by_comment_voter>();
    auto vote_itr = vote_idx.lower_bound(comment_id);
    while (vote_itr != vote_idx.end() && vote_itr->comment == comment_id)
    {
        ret.push_back(std::cref(*(vote_itr++)));
    }

    return ret;
}

dbs_comment_vote::comment_vote_refs_type
dbs_comment_vote::get_by_comment_weight_voter(const comment_id_type& comment_id) const
{
    comment_vote_refs_type ret;

    const auto& vote_idx = db_impl().get_index<comment_vote_index>().indices().get<by_comment_weight_voter>();
    auto vote_itr = vote_idx.lower_bound(comment_id);
    while (vote_itr != vote_idx.end() && vote_itr->comment == comment_id)
    {
        ret.push_back(std::cref(*(vote_itr++)));
    }

    return ret;
}

bool dbs_comment_vote::is_exists(const comment_id_type& comment_id, const account_id_type& voter_id) const
{
    return nullptr != db_impl().find<comment_vote_object, by_comment_voter>(std::make_tuple(comment_id, voter_id));
}

void dbs_comment_vote::remove_by_comment(const comment_id_type& comment_id)
{
    const auto& vote_idx = db_impl().get_index<comment_vote_index>().indices().get<by_comment_voter>();

    auto vote_itr = vote_idx.lower_bound(comment_id_type(comment_id));
    while (vote_itr != vote_idx.end() && vote_itr->comment == comment_id)
    {
        const auto& cur_vote = *vote_itr;
        ++vote_itr;
        remove(cur_vote);
    }
}

} // namespace chain
} // namespace scorum
