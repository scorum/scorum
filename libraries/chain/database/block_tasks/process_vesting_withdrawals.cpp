#include <scorum/chain/database/block_tasks/process_vesting_withdrawals.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_vesting_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_vesting_withdrawals::on_apply(block_task_context& ctx)
{
    account_service_i& account_service = ctx.services().account_service();
    dynamic_global_property_service_i& dprops_service = ctx.services().dynamic_global_property_service();
    withdraw_vesting_service_i& withdraw_vesting_service = ctx.services().withdraw_vesting_service();
    withdraw_vesting_route_service_i& withdraw_vesting_route_service = ctx.services().withdraw_vesting_route_service();

    for (const withdraw_vesting_object& wvo : withdraw_vesting_service.get_until(dprops_service.head_block_time()))
    {
        asset vesting_shares = asset(0, VESTS_SYMBOL);

        if (withdraw_vesting_objects::is_account(wvo.from_id))
        {
            const auto& from_account = account_service.get(wvo.from_id.get<account_id_type>());

            vesting_shares = from_account.vesting_shares;
        }

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

        asset deposited_vests = asset(0, VESTS_SYMBOL);

        for (const withdraw_vesting_route_object& wvro : withdraw_vesting_route_service.get_all(wvo.from_id))
        {
            asset to_deposit
                = asset((fc::uint128_t(to_withdraw.amount.value) * wvro.percent / SCORUM_100_PERCENT).to_uint64(),
                        VESTS_SYMBOL);

            if (to_deposit.amount > 0)
            {
                if (withdraw_vesting_objects::is_account(wvro.to_id))
                {
                    const auto& from_account = account_service.get(wvo.from_id.get<account_id_type>());
                    const auto& to_account = account_service.get(wvro.to_id.get<account_id_type>());

                    deposited_vests += to_deposit;

                    if (wvro.auto_vest) // withdraw SP
                    {
                        ctx.push_virtual_operation(
                            fill_vesting_withdraw_operation(from_account.name, to_account.name, to_deposit));

                        account_service.increase_vesting_shares(to_account, to_deposit);

                        account_service.adjust_proxied_witness_votes(to_account, to_deposit.amount);
                    }
                    else // convert SP to SCR and withdraw SCR
                    {
                        auto converted_scorum = asset(to_deposit.amount, SCORUM_SYMBOL);

                        ctx.push_virtual_operation(
                            fill_vesting_withdraw_operation(from_account.name, to_account.name, converted_scorum));

                        account_service.increase_balance(to_account, converted_scorum); //($)

                        dprops_service.update(
                            [&](dynamic_global_property_object& o) { o.total_vesting_shares -= to_deposit; });
                    }
                }
                else if (withdraw_vesting_objects::is_dev_committee(wvro.to_id))
                {
                    // TODO: dev pool
                }
            } // TODO: if percent to low to_deposit will vested to SCR like rest
        }

        // withdraw the rest as SCR

        if (withdraw_vesting_objects::is_account(wvo.from_id))
        {
            const auto& from_account = account_service.get(wvo.from_id.get<account_id_type>());

            asset to_convert = to_withdraw - deposited_vests;
            FC_ASSERT(to_convert.amount >= 0, "Deposited more vests than were supposed to be withdrawn");

            asset converted_scorum = asset(to_convert.amount, SCORUM_SYMBOL);

            ctx.push_virtual_operation(
                fill_vesting_withdraw_operation(from_account.name, from_account.name, converted_scorum));

            vesting_shares -= to_withdraw;

            account_service.update(from_account, [&](account_object& a) {
                a.vesting_shares = vesting_shares;
                a.balance += converted_scorum; //($)
            });

            withdraw_vesting_service.update(wvo, [&](withdraw_vesting_object& o) {
                o.withdrawn += to_withdraw;
                o.next_vesting_withdrawal += fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
            });

            if (wvo.withdrawn >= wvo.to_withdraw || vesting_shares.amount == 0)
            {
                withdraw_vesting_service.remove(wvo);
            }

            dprops_service.update([&](dynamic_global_property_object& o) { o.total_vesting_shares -= to_convert; });

            if (to_withdraw.amount > 0)
            {
                account_service.adjust_proxied_witness_votes(from_account, -to_withdraw.amount);
            }
        }
    }
}
}
}
}
