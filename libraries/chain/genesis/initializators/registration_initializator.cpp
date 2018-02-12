#include <scorum/chain/genesis/initializators/registration_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void registration_initializator_impl::apply(data_service_factory_i& services, const genesis_state_type& genesis_state)
{
    if (genesis_state.registration_schedule.empty() && genesis_state.registration_committee.empty()
        && !genesis_state.registration_supply.amount.value && !genesis_state.registration_bonus.amount.value)
    {
        dlog("No registration pool detected.");
        return;
    }

    // create sorted items list form genesis unordered data
    using schedule_item_type = registration_pool_object::schedule_item;
    using schedule_items_type = std::map<uint8_t, schedule_item_type>;
    schedule_items_type items;
    for (const auto& genesis_item : genesis_state.registration_schedule)
    {
        items.insert(schedule_items_type::value_type(
            genesis_item.stage, schedule_item_type{ genesis_item.users, genesis_item.bonus_percent }));
    }

    registration_pool_service_i& registration_pool_service = services.registration_pool_service();
    registration_committee_service_i& registration_committee_service = services.registration_committee_service();

    FC_ASSERT(!registration_pool_service.is_exists());

    registration_pool_service.create_pool(genesis_state.registration_supply, genesis_state.registration_bonus, items);

    using account_names_type = std::vector<account_name_type>;
    account_names_type committee;
    for (const auto& member : genesis_state.registration_committee)
    {
        committee.emplace_back(member);
    }
    registration_committee_service.create_committee(committee);
}
}
}
}
