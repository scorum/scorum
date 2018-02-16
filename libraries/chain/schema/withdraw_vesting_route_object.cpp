#include <scorum/chain/schema/withdraw_vesting_route_object.hpp>

namespace scorum {
namespace chain {

object_type get_withdraw_vesting_route_object_type(const withdraw_vesting_route_object_to_id_type& to_id)
{
    return (object_type)to_id.lo;
}
}
}
