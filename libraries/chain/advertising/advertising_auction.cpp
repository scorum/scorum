#include <scorum/chain/advertising/advertising_auction.hpp>

#include <scorum/chain/schema/budget_objects.hpp>

#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/algorithm/transform.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/advertising_property.hpp>
#include <scorum/chain/services/budgets.hpp>

#include <scorum/utils/range_adaptors.hpp>

namespace scorum {
namespace chain {

advertising_auction::advertising_auction(dynamic_global_property_service_i& dprops_svc,
                                         advertising_property_service_i& adv_props_svc,
                                         adv_budget_service_i<budget_type::post>& post_budget_svc,
                                         adv_budget_service_i<budget_type::banner>& banner_budget_svc)
    : _dprops_svc(dprops_svc)
    , _adv_props_svc(adv_props_svc)
    , _post_budget_svc(post_budget_svc)
    , _banner_budget_svc(banner_budget_svc)
{
}

void advertising_auction::run_round()
{
    const auto& post_coeffs = _adv_props_svc.get().auction_post_coefficients;
    const auto& banner_coeffs = _adv_props_svc.get().auction_banner_coefficients;

    run_round(_post_budget_svc, std::vector<percent_type>{ post_coeffs.begin(), post_coeffs.end() });
    run_round(_banner_budget_svc, std::vector<percent_type>{ banner_coeffs.begin(), banner_coeffs.end() });
}

template <budget_type budget_type_v>
void advertising_auction::run_round(adv_budget_service_i<budget_type_v>& budget_svc,
                                    const std::vector<percent_type>& coeffs)
{
    namespace br = boost::range;

    auto budgets = budget_svc.get_top_budgets(_dprops_svc.head_block_time());
    std::vector<asset> per_block_list;
    br::transform(budgets, std::back_inserter(per_block_list), [](auto b) { return b.get().per_block; });

    auto valuable_per_block_vec = utils::take_n(per_block_list, coeffs.size() + 1);
    auto auction_bets = calculate_bets(valuable_per_block_vec, coeffs);

    for (size_t i = 0; i < budgets.size(); ++i)
    {
        const auto& budget = budgets[i].get();

        auto per_block = budget_svc.allocate_cash(budget);

        auto adv_cash = asset(0, per_block.symbol());
        if (i < auction_bets.size())
            adv_cash = auction_bets[i];

        auto ret_cash = per_block - adv_cash;
        FC_ASSERT(ret_cash.amount >= 0, "Owner's cash-back amount cannot be less than zero");

        budget_svc.update_pending_payouts(budget, ret_cash, adv_cash);
    }
}

std::vector<asset> advertising_auction::calculate_bets(const std::vector<asset>& per_blocks,
                                                       const std::vector<percent_type>& coeffs)
{
    FC_ASSERT(coeffs.size() > 0, "invalid coefficient's list");
    FC_ASSERT(per_blocks.size() <= coeffs.size() + 1, "invalid list of per-block values");
    FC_ASSERT(std::is_sorted(coeffs.rbegin(), coeffs.rend()), "per-block list isn't sorted");
    FC_ASSERT(std::is_sorted(per_blocks.rbegin(), per_blocks.rend()), "Auction coefficients aren't sorted");
    FC_ASSERT(*coeffs.rbegin() > 0, "Auction coefficients should be positive");
    FC_ASSERT(*coeffs.begin() <= 100, "Auction coefficients should be less than 100");
    FC_ASSERT(per_blocks.empty() || per_blocks.rbegin()->amount > 0, "per-block amount should be positive");

    if (per_blocks.empty())
        return {};

    int64_t bets_size = std::min(per_blocks.size(), coeffs.size());
    std::vector<asset> bets;

    auto smallest_bet = bets_size < (int64_t)per_blocks.size() //
        ? per_blocks[bets_size]
        : per_blocks[bets_size - 1];
    bets.push_back(smallest_bet);

    for (int64_t ridx = bets_size - 2; ridx >= 0l; --ridx)
    {
        auto factor = utils::make_fraction((coeffs[ridx] - coeffs[ridx + 1]), coeffs.front());
        auto bet = bets.back() + per_blocks[ridx + 1] * factor;
        bets.push_back(std::min(bet, per_blocks[ridx]));
    }

    boost::reverse(bets);

    return bets;
}
}
}
