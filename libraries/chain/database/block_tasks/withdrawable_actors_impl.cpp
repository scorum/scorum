#include "withdrawable_actors_impl.hpp"

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/protocol/operations.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

struct asset_return_visitor
{
    typedef asset result_type;

    template <typename Op> asset operator()(const Op&) const
    {
        FC_ASSERT(false, "Not implemented.");
        return asset();
    }
};

class get_available_scorumpower_visitor : public asset_return_visitor
{
public:
    get_available_scorumpower_visitor(account_service_i& account_service, dev_pool_service_i& dev_pool_service)
        : _account_service(account_service)
        , _dev_pool_service(dev_pool_service)
    {
    }

    asset operator()(const account_id_type& id) const
    {
        const account_object& account = _account_service.get(id);

        return account.scorumpower;
    }

    asset operator()(const dev_committee_id_type& id) const
    {
        const dev_committee_object& pool = _dev_pool_service.get();

        FC_ASSERT(id == pool.id);

        return pool.sp_balance;
    }

private:
    account_service_i& _account_service;
    dev_pool_service_i& _dev_pool_service;
};

struct void_return_visitor
{
    typedef void result_type;

    template <typename Op> void operator()(const Op&) const
    {
        FC_ASSERT(false, "Not implemented.");
    }
};

class decrease_sp_visitor : public void_return_visitor
{
public:
    decrease_sp_visitor(account_service_i& account_service, dev_pool_service_i& dev_pool_service, const asset& amount)
        : _account_service(account_service)
        , _dev_pool_service(dev_pool_service)
        , _amount(amount)
    {
    }

    void operator()(const account_id_type& id) const
    {
        const account_object& from_account = _account_service.get(id);

        _account_service.decrease_scorumpower(from_account, _amount);
    }

    void operator()(const dev_committee_id_type& id) const
    {
        const dev_committee_object& from_pool = _dev_pool_service.get();

        FC_ASSERT(id == from_pool.id);

        _dev_pool_service.update([&](dev_committee_object& pool) { pool.sp_balance -= _amount; });
    }

private:
    account_service_i& _account_service;
    dev_pool_service_i& _dev_pool_service;
    const asset& _amount;
};

class increase_scr_visitor : public void_return_visitor
{
public:
    increase_scr_visitor(account_service_i& account_service, dev_pool_service_i& dev_pool_service, const asset& amount)
        : _account_service(account_service)
        , _dev_pool_service(dev_pool_service)
        , _amount(amount)
    {
    }

    void operator()(const account_id_type& to) const
    {
        const account_object& to_account = _account_service.get(to);

        _account_service.increase_balance(to_account, _amount);
    }

    void operator()(const dev_committee_id_type& to) const
    {
        const dev_committee_object& to_pool = _dev_pool_service.get();

        FC_ASSERT(to == to_pool.id);

        _dev_pool_service.update([&](dev_committee_object& pool) { pool.scr_balance += _amount; });
    }

private:
    account_service_i& _account_service;
    dev_pool_service_i& _dev_pool_service;
    const asset& _amount;
};

class increase_sp_visitor : public void_return_visitor
{
public:
    increase_sp_visitor(account_service_i& account_service, dev_pool_service_i& dev_pool_service, const asset& amount)
        : _account_service(account_service)
        , _dev_pool_service(dev_pool_service)
        , _amount(amount)
    {
    }

    void operator()(const account_id_type& id) const
    {
        const account_object& to_account = _account_service.get(id);

        _account_service.increase_scorumpower(to_account, _amount);
    }

    void operator()(const dev_committee_id_type& id) const
    {
        const dev_committee_object& to_pool = _dev_pool_service.get();

        FC_ASSERT(id == to_pool.id);

        _dev_pool_service.update([&](dev_committee_object& pool) { pool.sp_balance += _amount; });
    }

private:
    account_service_i& _account_service;
    dev_pool_service_i& _dev_pool_service;
    const asset& _amount;
};

class update_statistic_visitor : public void_return_visitor
{
    class update_statistic_from_account_visitor : public void_return_visitor
    {
    public:
        update_statistic_from_account_visitor(account_service_i& account_service,
                                              database_virtual_operations_emmiter_i& emmiter,
                                              const account_id_type& from,
                                              const asset& amount)
            : _account_service(account_service)
            , _emmiter(emmiter)
            , _from(from)
            , _amount(amount)
        {
        }

        void operator()(const account_id_type& to) const
        {
            const account_object& from_account = _account_service.get(_from);
            const account_object& to_account = _account_service.get(to);

            _emmiter.push_virtual_operation(
                acc_to_acc_vesting_withdraw_operation(from_account.name, to_account.name, _amount));
        }

        void operator()(const dev_committee_id_type& to) const
        {
            const account_object& from_account = _account_service.get(_from);

            _emmiter.push_virtual_operation(acc_to_devpool_vesting_withdraw_operation(from_account.name, _amount));
        }

    private:
        account_service_i& _account_service;
        database_virtual_operations_emmiter_i& _emmiter;
        const account_id_type& _from;
        const asset& _amount;
    };

    class update_statistic_from_dev_committee_visitor : public void_return_visitor
    {
    public:
        update_statistic_from_dev_committee_visitor(account_service_i& account_service,
                                                    database_virtual_operations_emmiter_i& emmiter,
                                                    const dev_committee_id_type& from,
                                                    const asset& amount)
            : _account_service(account_service)
            , _emmiter(emmiter)
            , _from(from)
            , _amount(amount)
        {
        }

        void operator()(const account_id_type& to) const
        {
            const account_object& to_account = _account_service.get(to);

            _emmiter.push_virtual_operation(devpool_to_acc_vesting_withdraw_operation(to_account.name, _amount));
        }

        void operator()(const dev_committee_id_type& to) const
        {
            _emmiter.push_virtual_operation(devpool_to_devpool_vesting_withdraw_operation(_amount));
        }

    private:
        account_service_i& _account_service;
        database_virtual_operations_emmiter_i& _emmiter;
        const dev_committee_id_type& _from;
        const asset& _amount;
    };

public:
    update_statistic_visitor(account_service_i& account_service,
                             database_virtual_operations_emmiter_i& emmiter,
                             const withdrawable_id_type& to,
                             const asset& amount)
        : _account_service(account_service)
        , _emmiter(emmiter)
        , _to(to)
        , _amount(amount)
    {
    }

    void operator()(const account_id_type& from) const
    {
        _to.visit(update_statistic_from_account_visitor(_account_service, _emmiter, from, _amount));
    }

    void operator()(const dev_committee_id_type& from) const
    {
        _to.visit(update_statistic_from_dev_committee_visitor(_account_service, _emmiter, from, _amount));
    }

private:
    account_service_i& _account_service;
    database_virtual_operations_emmiter_i& _emmiter;
    const withdrawable_id_type& _to;
    const asset& _amount;
};

// withdrawable_actors_impl
//

withdrawable_actors_impl::withdrawable_actors_impl(block_task_context& ctx)
    : _ctx(ctx)
    , _account_service(ctx.services().account_service())
    , _dev_pool_service(ctx.services().dev_pool_service())
    , _dprops_service(ctx.services().dynamic_global_property_service())
{
}

asset withdrawable_actors_impl::get_available_scorumpower(const withdrawable_id_type& from)
{
    return from.visit(get_available_scorumpower_visitor(_account_service, _dev_pool_service));
}

void withdrawable_actors_impl::update_statistic(const withdrawable_id_type& from,
                                                const withdrawable_id_type& to,
                                                const asset& amount)
{
    if (amount.amount > 0)
    {
        from.visit(update_statistic_visitor(_account_service, _ctx, to, amount));
    }
}

void withdrawable_actors_impl::update_statistic(const withdrawable_id_type& from)
{
    auto op = from.visit(
        [&](const account_id_type& from) {
            return protocol::operation(acc_finished_vesting_withdraw_operation(_account_service.get(from).name));
        },
        [&](const dev_committee_id_type& from) {
            return protocol::operation(devpool_finished_vesting_withdraw_operation());
        });

    _ctx.push_virtual_operation(op);
}

void withdrawable_actors_impl::increase_scr(const withdrawable_id_type& id, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SCORUM_SYMBOL);
    if (amount.amount > 0)
    {
        id.visit(increase_scr_visitor(_account_service, _dev_pool_service, amount));
    }
}

void withdrawable_actors_impl::increase_sp(const withdrawable_id_type& id, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SP_SYMBOL);
    if (amount.amount > 0)
    {
        id.visit(increase_sp_visitor(_account_service, _dev_pool_service, amount));
    }
}

void withdrawable_actors_impl::decrease_sp(const withdrawable_id_type& from, const asset& amount)
{
    FC_ASSERT(amount.symbol() == SP_SYMBOL);
    FC_ASSERT(amount.amount > 0);

    from.visit(decrease_sp_visitor(_account_service, _dev_pool_service, amount));
}
}
}
}
