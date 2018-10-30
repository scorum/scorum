#include <scorum/chain/services/matched_bet.hpp>

#include <boost/range/algorithm/copy.hpp>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/lambda/lambda.hpp>

namespace scorum {
namespace chain {

dbs_matched_bet::dbs_matched_bet(database& db)
    : base_service_type(db)
{
}

const matched_bet_object& dbs_matched_bet::get_matched_bets(const matched_bet_id_type& obj_id) const
{
    try
    {
        return get_by<by_id>(obj_id);
    }
    FC_CAPTURE_LOG_AND_RETHROW((obj_id))
}

dbs_matched_bet::view_type dbs_matched_bet::get_bets(matched_bet_id_type lower_bound) const
{
    auto& idx = db_impl().get_index<matched_bet_index, by_id>();
    return { idx.lower_bound(lower_bound), idx.end() };
}
} // namespace chain
} // namespace scorum
