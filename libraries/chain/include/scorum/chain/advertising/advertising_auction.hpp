#pragma once
#include <vector>
#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {

using protocol::budget_type;
using protocol::percent_type;
using protocol::asset;
struct dynamic_global_property_service_i;
struct advertising_property_service_i;
template <budget_type> class adv_budget_service_i;

/**
 * @details See [advertising details](@ref advdetails) for detailed information about how budgets work.
 */
class advertising_auction
{
public:
    advertising_auction(dynamic_global_property_service_i& dprops_svc,
                        advertising_property_service_i& adv_props_svc,
                        adv_budget_service_i<budget_type::post>& post_budget_svc,
                        adv_budget_service_i<budget_type::banner>& banner_budget_svc);

    void run_round();

    static std::vector<asset> calculate_bets(const std::vector<asset>& per_blocks,
                                             const std::vector<percent_type>& coeffs);

private:
    template <budget_type budget_type_v>
    void run_round(adv_budget_service_i<budget_type_v>& budget_svc, const std::vector<percent_type>& coeffs);

private:
    dynamic_global_property_service_i& _dprops_svc;
    advertising_property_service_i& _adv_props_svc;
    adv_budget_service_i<budget_type::post>& _post_budget_svc;
    adv_budget_service_i<budget_type::banner>& _banner_budget_svc;
};
}
}
