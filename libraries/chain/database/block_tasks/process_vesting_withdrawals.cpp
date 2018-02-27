#include <scorum/chain/database/block_tasks/process_vesting_withdrawals.hpp>

#include <scorum/chain/services/withdraw_vesting.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/withdraw_vesting_objects.hpp>

#include "withdrawable_actors_impl.hpp"

namespace scorum {
namespace chain {
namespace database_ns {

void process_vesting_withdrawals::on_apply(block_task_context& ctx)
{
    withdraw_vesting_service_i& withdraw_vesting_service = ctx.services().withdraw_vesting_service();
    withdraw_vesting_route_service_i& withdraw_vesting_route_service = ctx.services().withdraw_vesting_route_service();
    dynamic_global_property_service_i& dprops_service = ctx.services().dynamic_global_property_service();

    withdrawable_actors_impl actors_impl(ctx);

    for (const withdraw_vesting_object& wvo : withdraw_vesting_service.get_until(dprops_service.head_block_time()))
    {
        asset vesting_shares = actors_impl.get_available_vesting_shares(wvo.from_id);
        asset to_withdraw = asset(0, VESTS_SYMBOL);

        // use to_withdraw value in (0, wvo.vesting_withdraw_rate]
        if (wvo.to_withdraw - wvo.withdrawn < wvo.vesting_withdraw_rate)
        {
            to_withdraw = std::min(vesting_shares, wvo.to_withdraw % wvo.vesting_withdraw_rate);
        }
        else
        {
            to_withdraw = std::min(vesting_shares, wvo.vesting_withdraw_rate);
        }

        if (to_withdraw.amount <= 0) // invalid data for vesting
        {
            withdraw_vesting_service.remove(wvo);
            continue;
        }

        actors_impl.take_sp(wvo.from_id, to_withdraw);

        asset deposited_vests = asset(0, VESTS_SYMBOL);

        for (const withdraw_vesting_route_object& wvro : withdraw_vesting_route_service.get_all(wvo.from_id))
        {
            asset to_deposit
                = asset((fc::uint128_t(to_withdraw.amount.value) * wvro.percent / SCORUM_100_PERCENT).to_uint64(),
                        VESTS_SYMBOL);

            if (to_deposit.amount > 0)
            {
                deposited_vests += to_deposit;

                if (wvro.auto_vest) // withdraw SP
                {
                    actors_impl.give_sp(wvro.from_id, wvro.to_id, to_deposit);
                }
                else // convert SP to SCR and withdraw SCR
                {
                    auto converted_scorum = asset(to_deposit.amount, SCORUM_SYMBOL);

                    actors_impl.give_scr(wvro.from_id, wvro.to_id, converted_scorum);
                }
            }
        }

        // withdraw the rest as SCR or process vesting without routing

        asset to_convert = to_withdraw - deposited_vests;
        FC_ASSERT(to_convert.amount >= 0, "Deposited more vests than were supposed to be withdrawn");

        asset converted_scorum = asset(to_convert.amount, SCORUM_SYMBOL);

        actors_impl.give_scr(wvo.from_id, wvo.from_id, converted_scorum);

        vesting_shares -= to_withdraw;

        withdraw_vesting_service.update(wvo, [&](withdraw_vesting_object& o) {
            o.withdrawn += to_withdraw;
            o.next_vesting_withdrawal += fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
        });

        if (wvo.withdrawn >= wvo.to_withdraw || vesting_shares.amount == 0)
        {
            withdraw_vesting_service.remove(wvo);
        }
    }
}
}
}
}
