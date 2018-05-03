#include <scorum/chain/services/account_registration_bonus.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

#include <tuple>

namespace scorum {
namespace chain {

dbs_account_registration_bonus::dbs_account_registration_bonus(database& db)
    : base_service_type(db)
{
}

void dbs_account_registration_bonus::remove_if_exist(const account_name_type& name)
{
    const account_registration_bonus_object* pobj = find_by<by_account>(name);
    if (pobj)
    {
        remove(*pobj);
    }
}

dbs_account_registration_bonus::account_registration_bonus_refs_type
dbs_account_registration_bonus::get_by_expiration_time(const fc::time_point_sec& until) const
{
    try
    {
        return get_range_by<by_expiration>(::boost::multi_index::unbounded,
                                           ::boost::lambda::_1 <= std::make_tuple(until, ALL_IDS));
    }
    FC_CAPTURE_AND_RETHROW((until))
}
}
}
