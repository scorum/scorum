#include <scorum/chain/genesis/initializators/registration_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void registration_initializator_impl::on_apply(initializator_context& ctx)
{
    if (!is_registration_pool_exists(ctx))
    {
        dlog("There is no registration pool.");
        return;
    }

    FC_ASSERT(ctx.genesis_state().rewards_supply.symbol() == SCORUM_SYMBOL);
    FC_ASSERT(ctx.genesis_state().registration_supply.symbol() == SCORUM_SYMBOL);

    // create sorted items list form genesis unordered data
    using schedule_item_type = registration_pool_object::schedule_item;
    using schedule_items_type = std::map<uint8_t, schedule_item_type>;
    schedule_items_type items;
    for (const auto& genesis_item : ctx.genesis_state().registration_schedule)
    {
        items.insert(schedule_items_type::value_type(
            genesis_item.stage, schedule_item_type{ genesis_item.users, genesis_item.bonus_percent }));
    }

    registration_pool_service_i& registration_pool_service = ctx.services().registration_pool_service();
    registration_committee_service_i& registration_committee_service = ctx.services().registration_committee_service();

    FC_ASSERT(!registration_pool_service.is_exists());

    registration_pool_service.create_pool(ctx.genesis_state().registration_supply,
                                          ctx.genesis_state().registration_bonus, items);

    using account_names_type = std::vector<account_name_type>;
    account_names_type committee;
    for (const auto& member : ctx.genesis_state().registration_committee)
    {
        committee.emplace_back(member);
    }
    registration_committee_service.create_committee(committee);
}

bool registration_initializator_impl::is_registration_pool_exists(initializator_context& ctx)
{
    return ctx.genesis_state().registration_supply.amount.value || ctx.genesis_state().registration_bonus.amount.value
        || !ctx.genesis_state().registration_schedule.empty() || !ctx.genesis_state().registration_committee.empty();
}
}
}
}
