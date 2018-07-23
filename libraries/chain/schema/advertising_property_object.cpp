#include <scorum/chain/schema/advertising_property_object.hpp>

namespace scorum {
namespace chain {

template <>
const fc::shared_vector<percent_type>& advertising_property_object::get_vcg_coefficients<budget_type::post>() const
{
    return vcg_post_coefficients;
}

template <>
const fc::shared_vector<percent_type>& advertising_property_object::get_vcg_coefficients<budget_type::banner>() const
{
    return vcg_banner_coefficients;
}
}
}