#include "withdrawable_actors_impl.hpp"

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

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

class get_available_vesting_shares_visitor : public asset_return_visitor
{
public:
    get_available_vesting_shares_visitor(account_service_i& account_service, dev_pool_service_i& dev_pool_service)
        : _account_service(account_service)
        , _dev_pool_service(dev_pool_service)
    {
    }

    asset operator()(const account_id_type& id) const
    {
        const account_object& account = _account_service.get(id);

        return account.vesting_shares;
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

class give_scr_visitor : public void_return_visitor
{
    class give_scr_from_account_visitor : public void_return_visitor
    {
    public:
        give_scr_from_account_visitor(account_service_i& account_service,
                                      dev_pool_service_i& dev_pool_service,
                                      dynamic_global_property_service_i& dprops_service,
                                      database_virtual_operations_emmiter_i& emmiter,
                                      const account_id_type& from,
                                      const asset& amount)
            : _account_service(account_service)
            , _dev_pool_service(dev_pool_service)
            , _dprops_service(dprops_service)
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
                fill_vesting_withdraw_operation(from_account.name, to_account.name, _amount));

            _account_service.increase_balance(to_account, _amount);

            _dprops_service.update([&](dynamic_global_property_object& o) {
                o.total_vesting_shares -= asset(_amount.amount, VESTS_SYMBOL);
            });
        }

        void operator()(const dev_committee_id_type& to) const
        {
            const dev_committee_object& to_pool = _dev_pool_service.get();

            FC_ASSERT(to == to_pool.id);

            // TODO: statistic from account_id_type to dev_committee_id_type

            _dev_pool_service.update([&](dev_committee_object& pool) { pool.scr_balance += _amount; });

            _dprops_service.update([&](dynamic_global_property_object& o) {
                o.total_vesting_shares -= asset(_amount.amount, VESTS_SYMBOL);
                o.circulating_capital -= _amount;
            });
        }

    private:
        account_service_i& _account_service;
        dev_pool_service_i& _dev_pool_service;
        dynamic_global_property_service_i& _dprops_service;
        database_virtual_operations_emmiter_i& _emmiter;
        const account_id_type& _from;
        const asset& _amount;
    };

    class give_scr_from_dev_committee_visitor : public void_return_visitor
    {
    public:
        give_scr_from_dev_committee_visitor(account_service_i& account_service,
                                            dev_pool_service_i& dev_pool_service,
                                            dynamic_global_property_service_i& dprops_service,
                                            database_virtual_operations_emmiter_i& emmiter,
                                            const dev_committee_id_type& from,
                                            const asset& amount)
            : _account_service(account_service)
            , _dev_pool_service(dev_pool_service)
            , _dprops_service(dprops_service)
            , _emmiter(emmiter)
            , _from(from)
            , _amount(amount)
        {
        }

        void operator()(const account_id_type& to) const
        {
            const account_object& to_account = _account_service.get(to);

            // TODO: statistic from dev_committee_id_type to account_id_type

            _account_service.increase_balance(to_account, _amount);

            _dprops_service.update([&](dynamic_global_property_object& o) {
                o.circulating_capital += asset(_amount.amount, SCORUM_SYMBOL);
            });
        }

        void operator()(const dev_committee_id_type& to) const
        {
            const dev_committee_object& to_pool = _dev_pool_service.get();

            FC_ASSERT(to == to_pool.id);

            // TODO: statistic from dev_committee_id_type to dev_committee_id_type

            _dev_pool_service.update([&](dev_committee_object& pool) { pool.scr_balance += _amount; });
        }

    private:
        account_service_i& _account_service;
        dev_pool_service_i& _dev_pool_service;
        dynamic_global_property_service_i& _dprops_service;
        database_virtual_operations_emmiter_i& _emmiter;
        const dev_committee_id_type& _from;
        const asset& _amount;
    };

public:
    give_scr_visitor(account_service_i& account_service,
                     dev_pool_service_i& dev_pool_service,
                     dynamic_global_property_service_i& dprops_service,
                     database_virtual_operations_emmiter_i& emmiter,
                     const withdrawable_id_type& to,
                     const asset& amount)
        : _account_service(account_service)
        , _dev_pool_service(dev_pool_service)
        , _dprops_service(dprops_service)
        , _emmiter(emmiter)
        , _to(to)
        , _amount(amount)
    {
    }

    void operator()(const account_id_type& from) const
    {
        _to.visit(give_scr_from_account_visitor(_account_service, _dev_pool_service, _dprops_service, _emmiter, from,
                                                _amount));
    }

    void operator()(const dev_committee_id_type& from) const
    {
        _to.visit(give_scr_from_dev_committee_visitor(_account_service, _dev_pool_service, _dprops_service, _emmiter,
                                                      from, _amount));
    }

private:
    account_service_i& _account_service;
    dev_pool_service_i& _dev_pool_service;
    dynamic_global_property_service_i& _dprops_service;
    database_virtual_operations_emmiter_i& _emmiter;
    const withdrawable_id_type& _to;
    const asset& _amount;
};

class give_sp_visitor : public void_return_visitor
{
    class give_sp_from_account_visitor : public void_return_visitor
    {
    public:
        give_sp_from_account_visitor(account_service_i& account_service,
                                     dev_pool_service_i& dev_pool_service,
                                     dynamic_global_property_service_i& dprops_service,
                                     database_virtual_operations_emmiter_i& emmiter,
                                     const account_id_type& from,
                                     const asset& amount)
            : _account_service(account_service)
            , _dev_pool_service(dev_pool_service)
            , _dprops_service(dprops_service)
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
                fill_vesting_withdraw_operation(from_account.name, to_account.name, _amount));

            _account_service.increase_vesting_shares(to_account, _amount);

            _account_service.adjust_proxied_witness_votes(to_account, _amount.amount);
        }

        void operator()(const dev_committee_id_type& to) const
        {
            const dev_committee_object& to_pool = _dev_pool_service.get();

            FC_ASSERT(to == to_pool.id);

            // TODO: statistic from account_id_type to dev_committee_id_type

            _dev_pool_service.update([&](dev_committee_object& pool) { pool.sp_balance += _amount; });

            _dprops_service.update([&](dynamic_global_property_object& o) {
                o.total_vesting_shares -= _amount;
                o.circulating_capital -= asset(_amount.amount, SCORUM_SYMBOL);
            });
        }

    private:
        account_service_i& _account_service;
        dev_pool_service_i& _dev_pool_service;
        dynamic_global_property_service_i& _dprops_service;
        database_virtual_operations_emmiter_i& _emmiter;
        const account_id_type& _from;
        const asset& _amount;
    };

    class give_sp_from_dev_committee_visitor : public void_return_visitor
    {
    public:
        give_sp_from_dev_committee_visitor(account_service_i& account_service,
                                           dev_pool_service_i& dev_pool_service,
                                           dynamic_global_property_service_i& dprops_service,
                                           database_virtual_operations_emmiter_i& emmiter,
                                           const dev_committee_id_type& from,
                                           const asset& amount)
            : _account_service(account_service)
            , _dev_pool_service(dev_pool_service)
            , _dprops_service(dprops_service)
            , _emmiter(emmiter)
            , _from(from)
            , _amount(amount)
        {
        }

        void operator()(const account_id_type& to) const
        {
            const account_object& to_account = _account_service.get(to);

            // TODO: statistic from dev_committee_id_type to account_id_type

            _account_service.increase_vesting_shares(to_account, _amount);

            _account_service.adjust_proxied_witness_votes(to_account, _amount.amount);

            _dprops_service.update([&](dynamic_global_property_object& o) {
                o.total_vesting_shares += _amount;
                o.circulating_capital += asset(_amount.amount, SCORUM_SYMBOL);
            });
        }

        void operator()(const dev_committee_id_type& to) const
        {
            const dev_committee_object& to_pool = _dev_pool_service.get();

            FC_ASSERT(to == to_pool.id);

            // TODO: statistic from dev_committee_id_type to dev_committee_id_type

            _dev_pool_service.update([&](dev_committee_object& pool) { pool.sp_balance += _amount; });
        }

    private:
        account_service_i& _account_service;
        dev_pool_service_i& _dev_pool_service;
        dynamic_global_property_service_i& _dprops_service;
        database_virtual_operations_emmiter_i& _emmiter;
        const dev_committee_id_type& _from;
        const asset& _amount;
    };

public:
    give_sp_visitor(account_service_i& account_service,
                    dev_pool_service_i& dev_pool_service,
                    dynamic_global_property_service_i& dprops_service,
                    database_virtual_operations_emmiter_i& emmiter,
                    const withdrawable_id_type& to,
                    const asset& amount)
        : _account_service(account_service)
        , _dev_pool_service(dev_pool_service)
        , _dprops_service(dprops_service)
        , _emmiter(emmiter)
        , _to(to)
        , _amount(amount)
    {
    }

    void operator()(const account_id_type& from) const
    {
        _to.visit(give_sp_from_account_visitor(_account_service, _dev_pool_service, _dprops_service, _emmiter, from,
                                               _amount));
    }

    void operator()(const dev_committee_id_type& from) const
    {
        _to.visit(give_sp_from_dev_committee_visitor(_account_service, _dev_pool_service, _dprops_service, _emmiter,
                                                     from, _amount));
    }

private:
    account_service_i& _account_service;
    dev_pool_service_i& _dev_pool_service;
    dynamic_global_property_service_i& _dprops_service;
    database_virtual_operations_emmiter_i& _emmiter;
    const withdrawable_id_type& _to;
    const asset& _amount;
};

class take_sp_visitor : public void_return_visitor
{
public:
    take_sp_visitor(account_service_i& account_service, dev_pool_service_i& dev_pool_service, const asset& amount)
        : _account_service(account_service)
        , _dev_pool_service(dev_pool_service)
        , _amount(amount)
    {
    }

    void operator()(const account_id_type& id) const
    {
        const account_object& from_account = _account_service.get(id);

        _account_service.update(from_account, [&](account_object& a) { a.vesting_shares -= _amount; });

        _account_service.adjust_proxied_witness_votes(from_account, -_amount.amount);
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

// withdrawable_actors_impl
//

withdrawable_actors_impl::withdrawable_actors_impl(block_task_context& ctx)
    : _ctx(ctx)
    , _account_service(ctx.services().account_service())
    , _dev_pool_service(ctx.services().dev_pool_service())
    , _dprops_service(ctx.services().dynamic_global_property_service())
{
}

asset withdrawable_actors_impl::get_available_vesting_shares(const withdrawable_id_type& from)
{
    return from.visit(get_available_vesting_shares_visitor(_account_service, _dev_pool_service));
}

void withdrawable_actors_impl::give_scr(const withdrawable_id_type& from,
                                        const withdrawable_id_type& to,
                                        const asset& amount)
{
    FC_ASSERT(amount.symbol() == SCORUM_SYMBOL);
    if (amount.amount > 0)
    {
        from.visit(give_scr_visitor(_account_service, _dev_pool_service, _dprops_service, _ctx, to, amount));
    }
}

void withdrawable_actors_impl::give_sp(const withdrawable_id_type& from,
                                       const withdrawable_id_type& to,
                                       const asset& amount)
{
    FC_ASSERT(amount.symbol() == VESTS_SYMBOL);
    if (amount.amount > 0)
    {
        from.visit(give_sp_visitor(_account_service, _dev_pool_service, _dprops_service, _ctx, to, amount));
    }
}

void withdrawable_actors_impl::take_sp(const withdrawable_id_type& from, const asset& amount)
{
    FC_ASSERT(amount.symbol() == VESTS_SYMBOL);
    FC_ASSERT(amount.amount > 0);

    from.visit(take_sp_visitor(_account_service, _dev_pool_service, amount));
}
}
}
}
