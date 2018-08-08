#include <scorum/chain/services/matched_bet.hpp>

#include <boost/range/algorithm/copy.hpp>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

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

void dbs_matched_bet::foreach_bets(const bet_id_type& bet_id, dbs_matched_bet::matched_bet_call_type call) const
{
    try
    {
        auto matched_bet1
            = get_range_by<by_matched_bet1_id>(bet_id <= ::boost::lambda::_1, ::boost::lambda::_1 <= bet_id);
        auto matched_bet2
            = get_range_by<by_matched_bet2_id>(bet_id <= ::boost::lambda::_1, ::boost::lambda::_1 <= bet_id);

        struct less_by_id
        {
            bool operator()(matched_bet_service_i::object_cref_type lhs,
                            matched_bet_service_i::object_cref_type rhs) const
            {
                return lhs.get().id < rhs.get().id;
            }
        };

        std::set<matched_bet_service_i::object_cref_type, less_by_id> matched_set;

        boost::copy(matched_bet1, std::inserter(matched_set, end(matched_set)));
        boost::copy(matched_bet2, std::inserter(matched_set, end(matched_set)));

        for (const matched_bet_object& matched_obj : matched_set)
        {
            call(matched_obj);
        }
    }
    FC_CAPTURE_LOG_AND_RETHROW((bet_id))
}
}
}
