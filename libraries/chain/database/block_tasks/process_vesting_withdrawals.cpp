#include <scorum/chain/database/block_tasks/process_vesting_withdrawals.hpp>

#include <scorum/chain/services/withdraw_vesting.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/schema/withdraw_vesting_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

class withdrawable_actors_impl // this class implements {withdrawable_id_type} specific only
{
public:
    explicit withdrawable_actors_impl(block_task_context& ctx)
        : _ctx(ctx)
        , _account_service(ctx.services().account_service())
        , _dev_pool_service(ctx.services().dev_pool_service())
    {
    }

    asset get_available_vesting_shares(const withdrawable_id_type& from)
    {
        if (withdraw_vesting_objects::is_account(from))
            return get_available_vesting_shares(from.get<account_id_type>());
        else if (withdraw_vesting_objects::is_dev_committee(from))
            return get_available_vesting_shares(from.get<dev_committee_id_type>());

        return asset(0, VESTS_SYMBOL);
    }

    void give_scr_to(const withdrawable_id_type& from, const withdrawable_id_type& to, const asset& amount)
    {
        FC_ASSERT(amount.symbol() == SCORUM_SYMBOL);

        if (withdraw_vesting_objects::is_account(from) && withdraw_vesting_objects::is_account(to))
            give_scr_to(from.get<account_id_type>(), to.get<account_id_type>(), amount);
        else if (withdraw_vesting_objects::is_dev_committee(from) && withdraw_vesting_objects::is_dev_committee(to))
            give_scr_to(from.get<dev_committee_id_type>(), to.get<dev_committee_id_type>(), amount);
        else if (withdraw_vesting_objects::is_account(from) && withdraw_vesting_objects::is_dev_committee(to))
            give_scr_to(from.get<account_id_type>(), to.get<dev_committee_id_type>(), amount);
        else if (withdraw_vesting_objects::is_dev_committee(from) && withdraw_vesting_objects::is_account(to))
            give_scr_to(from.get<dev_committee_id_type>(), to.get<account_id_type>(), amount);
        else
        {
            FC_ASSERT(false, "Unknown type");
        }
    }

    void give_sp_to(const withdrawable_id_type& from, const withdrawable_id_type& to, const asset& amount)
    {
        FC_ASSERT(amount.symbol() == VESTS_SYMBOL);

        if (withdraw_vesting_objects::is_account(from) && withdraw_vesting_objects::is_account(to))
            give_sp_to(from.get<account_id_type>(), to.get<account_id_type>(), amount);
        else if (withdraw_vesting_objects::is_dev_committee(from) && withdraw_vesting_objects::is_dev_committee(to))
            give_sp_to(from.get<dev_committee_id_type>(), to.get<dev_committee_id_type>(), amount);
        else if (withdraw_vesting_objects::is_account(from) && withdraw_vesting_objects::is_dev_committee(to))
            give_sp_to(from.get<account_id_type>(), to.get<dev_committee_id_type>(), amount);
        else if (withdraw_vesting_objects::is_dev_committee(from) && withdraw_vesting_objects::is_account(to))
            give_sp_to(from.get<dev_committee_id_type>(), to.get<account_id_type>(), amount);
        else
        {
            FC_ASSERT(false, "Unknown type");
        }
    }

    void pick_up_from(const withdrawable_id_type& from, const asset& to_withdraw, const asset& converted)
    {
        FC_ASSERT(to_withdraw.symbol() == VESTS_SYMBOL);
        FC_ASSERT(converted.symbol() == SCORUM_SYMBOL);

        if (withdraw_vesting_objects::is_account(from))
            pick_up_from(from.get<account_id_type>(), to_withdraw, converted);
        else if (withdraw_vesting_objects::is_dev_committee(from))
            pick_up_from(from.get<dev_committee_id_type>(), to_withdraw, converted);
        else
        {
            FC_ASSERT(false, "Unknown type");
        }
    }

private:
    asset get_available_vesting_shares(const account_id_type& from)
    {
        const account_object& from_account = _account_service.get(from);

        return from_account.vesting_shares;
    }

    asset get_available_vesting_shares(const dev_committee_id_type& from)
    {
        const dev_committee_object& from_pool = _dev_pool_service.get();

        FC_ASSERT(from == from_pool.id);

        return from_pool.balance_in;
    }

    void give_scr_to(const account_id_type& from, const account_id_type& to, const asset& amount)
    {
        const account_object& from_account = _account_service.get(from);
        const account_object& to_account = _account_service.get(to);

        _ctx.push_virtual_operation(fill_vesting_withdraw_operation(from_account.name, to_account.name, amount));

        _account_service.increase_balance(to_account, amount);
    }
    void give_sp_to(const account_id_type& from, const account_id_type& to, const asset& amount)
    {
        const account_object& from_account = _account_service.get(from);
        const account_object& to_account = _account_service.get(to);

        _ctx.push_virtual_operation(fill_vesting_withdraw_operation(from_account.name, to_account.name, amount));

        _account_service.increase_vesting_shares(to_account, amount);

        _account_service.adjust_proxied_witness_votes(to_account, amount.amount);
    }

    void give_scr_to(const dev_committee_id_type& from, const dev_committee_id_type& to, const asset& amount)
    {
        const dev_committee_object& to_pool = _dev_pool_service.get();

        FC_ASSERT(to == to_pool.id);

        // TODO: statistic

        _dev_pool_service.update([&](dev_committee_object& pool) { pool.balance_out += amount; });
    }
    void give_sp_to(const dev_committee_id_type& from, const dev_committee_id_type& to, const asset& amount)
    {
        const dev_committee_object& to_pool = _dev_pool_service.get();

        FC_ASSERT(to == to_pool.id);

        // TODO: statistic

        _dev_pool_service.update([&](dev_committee_object& pool) { pool.balance_in += amount; });
    }

    void give_scr_to(const account_id_type& from, const dev_committee_id_type& to, const asset& amount)
    {
        const dev_committee_object& to_pool = _dev_pool_service.get();

        FC_ASSERT(to == to_pool.id);

        // TODO: statistic

        _dev_pool_service.update([&](dev_committee_object& pool) { pool.balance_out += amount; });
    }
    void give_sp_to(const account_id_type& from, const dev_committee_id_type& to, const asset& amount)
    {
        const dev_committee_object& to_pool = _dev_pool_service.get();

        FC_ASSERT(to == to_pool.id);

        // TODO: statistic

        _dev_pool_service.update([&](dev_committee_object& pool) { pool.balance_in += amount; });
    }

    void give_scr_to(const dev_committee_id_type& from, const account_id_type& to, const asset& amount)
    {
        const account_object& to_account = _account_service.get(to);

        // TODO: statistic

        _account_service.increase_balance(to_account, amount);
    }
    void give_sp_to(const dev_committee_id_type& from, const account_id_type& to, const asset& amount)
    {
        const account_object& to_account = _account_service.get(to);

        // TODO: statistic

        _account_service.increase_vesting_shares(to_account, amount);

        _account_service.adjust_proxied_witness_votes(to_account, amount.amount);
    }

    void pick_up_from(const account_id_type& from, const asset& to_withdraw, const asset& converted)
    {
        FC_ASSERT(to_withdraw.amount > 0);

        const account_object& from_account = _account_service.get(from);

        _ctx.push_virtual_operation(fill_vesting_withdraw_operation(from_account.name, from_account.name, converted));

        _account_service.update(from_account, [&](account_object& a) {
            a.vesting_shares -= to_withdraw;
            a.balance += converted; //($)
        });

        _account_service.adjust_proxied_witness_votes(from_account, -to_withdraw.amount);
    }

    void pick_up_from(const dev_committee_id_type& from, const asset& to_withdraw, const asset& converted)
    {
        const dev_committee_object& from_pool = _dev_pool_service.get();

        FC_ASSERT(from == from_pool.id);

        // TODO: statistic

        _dev_pool_service.update([&](dev_committee_object& pool) {
            pool.balance_in -= to_withdraw;
            pool.balance_out += converted; //($)
        });
    }

    block_task_context& _ctx;
    account_service_i& _account_service;
    dev_pool_service_i& _dev_pool_service;
};

//

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
                    actors_impl.give_sp_to(wvro.from_id, wvro.to_id, to_deposit);
                }
                else // convert SP to SCR and withdraw SCR
                {
                    auto converted_scorum = asset(to_deposit.amount, SCORUM_SYMBOL);

                    actors_impl.give_scr_to(wvro.from_id, wvro.to_id, converted_scorum);

                    dprops_service.update(
                        [&](dynamic_global_property_object& o) { o.total_vesting_shares -= to_deposit; });
                }
            }
        }

        // withdraw the rest as SCR or process vesting without routing

        asset to_convert = to_withdraw - deposited_vests;
        FC_ASSERT(to_convert.amount >= 0, "Deposited more vests than were supposed to be withdrawn");

        asset converted_scorum = asset(to_convert.amount, SCORUM_SYMBOL);

        actors_impl.pick_up_from(wvo.from_id, to_withdraw, converted_scorum);

        dprops_service.update([&](dynamic_global_property_object& o) { o.total_vesting_shares -= to_convert; });

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
