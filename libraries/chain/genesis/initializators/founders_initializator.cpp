#include <scorum/chain/genesis/initializators/founders_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <fc/exception/exception.hpp>
#include <fc/uint128.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <limits>

namespace scorum {
namespace chain {
namespace genesis {

void founders_initializator_impl::on_apply(initializator_context& ctx)
{
    if (!is_founders_pool_exists(ctx))
    {
        dlog("There is no founders supply.");
        return;
    }

    FC_ASSERT(SCORUM_100_PERCENT <= std::numeric_limits<uint16_t>::max()); // constant value check

    FC_ASSERT(ctx.genesis_state().founders_supply.symbol() == SP_SYMBOL);

    check_founders(ctx);

    account_name_type pitiful;
    asset founders_supply_rest = distribure_sp_by_percent(ctx, pitiful);
    if (founders_supply_rest.amount > (share_value_type)0)
    {
        distribure_sp_rest(ctx, founders_supply_rest, pitiful);
    }
}

bool founders_initializator_impl::is_founders_pool_exists(initializator_context& ctx)
{
    return ctx.genesis_state().founders_supply.amount.value || !ctx.genesis_state().founders.empty();
}

void founders_initializator_impl::check_founders(initializator_context& ctx)
{
    account_service_i& account_service = ctx.services().account_service();

    uint16_t total_sp_percent = 0u;
    for (auto& founder : ctx.genesis_state().founders)
    {
        FC_ASSERT(!founder.name.empty(), "Founder 'name' should not be empty.");

        account_service.check_account_existence(founder.name);

        FC_ASSERT(founder.sp_percent >= 0.f && founder.sp_percent <= 100.f,
                  "Founder 'sp_percent' should be in range [0, 100]. ${1} received.", ("1", founder.sp_percent));

        total_sp_percent += SCORUM_PERCENT(founder.sp_percent);

        FC_ASSERT(total_sp_percent <= (uint16_t)SCORUM_100_PERCENT, "Total 'sp_percent' more then 100%.");
    }
    FC_ASSERT(total_sp_percent > 0u, "Total 'sp_percent' less or equal zerro.");
}

asset founders_initializator_impl::distribure_sp_by_percent(initializator_context& ctx, account_name_type& pitiful)
{
    asset founders_supply_rest = ctx.genesis_state().founders_supply;
    for (auto& founder : ctx.genesis_state().founders)
    {
        uint16_t percent = SCORUM_PERCENT(founder.sp_percent);

        asset sp_bonus = ctx.genesis_state().founders_supply;

        fc::uint128_t sp_bonus_amount = sp_bonus.amount.value;
        sp_bonus_amount *= percent;
        sp_bonus_amount /= SCORUM_100_PERCENT;
        sp_bonus.amount = sp_bonus_amount.to_uint64();

        if (sp_bonus.amount > (share_value_type)0)
        {
            feed_account(ctx, founder.name, sp_bonus);
            founders_supply_rest -= sp_bonus;
        }
        else if (founder.sp_percent > 0.f)
        {
            pitiful = founder.name; // only last pitiful is getting SP
        }
    }
    return founders_supply_rest;
}

void founders_initializator_impl::distribure_sp_rest(initializator_context& ctx,
                                                     const asset& rest,
                                                     const account_name_type& pitiful)
{
    static const float sp_percent_limit_for_pitiful = 0.02f;

    FC_ASSERT(rest.amount < ctx.genesis_state().founders_supply.amount * SCORUM_PERCENT(sp_percent_limit_for_pitiful)
                      / SCORUM_100_PERCENT,
              "Too big rest ${r} for single pitiful. There are many pitiful members in genesis maybe.", ("r", rest));

    asset founders_supply_rest = rest;
    if (pitiful != account_name_type())
    {
        feed_account(ctx, pitiful, founders_supply_rest);
        founders_supply_rest.amount = 0;
    }

    FC_ASSERT(!founders_supply_rest.amount.value);
}

void founders_initializator_impl::feed_account(initializator_context& ctx,
                                               const account_name_type& name,
                                               const asset& sp)
{
    account_service_i& account_service = ctx.services().account_service();

    const auto& founder_obj = account_service.get_account(name);
    account_service.increase_scorumpower(founder_obj, sp);
}
}
}
}
