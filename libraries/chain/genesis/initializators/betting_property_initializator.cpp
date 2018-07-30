#include <scorum/chain/genesis/initializators/betting_property_initializator.hpp>

#include <scorum/chain/services/betting_property.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void betting_property_initializator_impl::on_apply(initializator_context& ctx)
{
    auto& service = ctx.services().betting_property_service();

    FC_ASSERT(!service.is_exists());

    service.create([&](betting_property_object& obj) { obj.moderator = SCORUM_MISSING_MODERATOR_ACCOUNT; });
}

} // namespace genesis
} // namespace scorum
} // namespace chain
