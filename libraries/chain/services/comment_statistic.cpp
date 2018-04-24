#include <scorum/chain/services/comment_statistic.hpp>

namespace scorum {
namespace chain {

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
}
}
