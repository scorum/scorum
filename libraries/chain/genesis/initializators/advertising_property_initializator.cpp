#include <scorum/chain/genesis/initializators/advertising_property_initializator.hpp>

#include <scorum/chain/services/advertising_property.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void advertising_property_initializator_impl::on_apply(initializator_context& ctx)
{
    auto& adv_service = ctx.services().advertising_property_service();

    FC_ASSERT(!adv_service.is_exists());

    adv_service.create([&](advertising_property_object& obj) {
        obj.moderator = SCORUM_MISSING_MODERATOR_ACCOUNT;
        std::vector<percent_type> coeffs(SCORUM_DEFAULT_BUDGETS_AUCTION_SET);
        std::copy(std::begin(coeffs), std::end(coeffs), std::back_inserter(obj.auction_post_coefficients));
        std::copy(std::begin(coeffs), std::end(coeffs), std::back_inserter(obj.auction_banner_coefficients));
    });
}

} // namespace genesis
} // namespace scorum
} // namespace chain
