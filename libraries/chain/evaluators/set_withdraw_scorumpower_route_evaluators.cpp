#include <scorum/chain/evaluators/set_withdraw_scorumpower_route_evaluators.hpp>

#include <fc/exception/exception.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/withdraw_scorumpower_route.hpp>
#include <scorum/chain/services/withdraw_scorumpower_route_statistic.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

namespace scorum {
namespace chain {

class set_withdraw_scorumpower_route_evaluator_impl
{
public:
    set_withdraw_scorumpower_route_evaluator_impl(data_service_factory_i& services)
        : _withdraw_scorumpower_route_service(services.withdraw_scorumpower_route_service())
        , _withdraw_scorumpower_route_statistic_service(services.withdraw_scorumpower_route_statistic_service())
    {
    }

    template <typename FromObjectType, typename ToObjectType>
    void do_apply(const FromObjectType& from_object,
                  const ToObjectType& to_object,
                  const uint16_t percent,
                  bool auto_vest = false)
    {
        if (!_withdraw_scorumpower_route_statistic_service.is_exists(from_object.id))
        {
            _withdraw_scorumpower_route_statistic_service.create(
                [&](withdraw_scorumpower_route_statistic_object& wvdso) { wvdso.from_id = from_object.id; });
        }

        const withdraw_scorumpower_route_statistic_object& statistic
            = _withdraw_scorumpower_route_statistic_service.get(from_object.id);

        if (!_withdraw_scorumpower_route_service.is_exists(from_object.id, to_object.id))
        {
            FC_ASSERT(percent != 0, "Cannot create a 0% destination.");

            FC_ASSERT(statistic.withdraw_routes < SCORUM_MAX_WITHDRAW_ROUTES,
                      "Account already has the maximum number of routes.");

            _withdraw_scorumpower_route_service.create([&](withdraw_scorumpower_route_object& wvdo) {
                wvdo.from_id = from_object.id;
                wvdo.to_id = to_object.id;
                wvdo.percent = percent;
                wvdo.auto_vest = auto_vest;
            });

            _withdraw_scorumpower_route_statistic_service.update(
                statistic, [&](withdraw_scorumpower_route_statistic_object& wvdso) { wvdso.withdraw_routes++; });
        }
        else if (percent == 0)
        {
            const auto& wvr = _withdraw_scorumpower_route_service.get(from_object.id, to_object.id);
            _withdraw_scorumpower_route_service.remove(wvr);

            _withdraw_scorumpower_route_statistic_service.update(
                statistic, [&](withdraw_scorumpower_route_statistic_object& wvdso) { wvdso.withdraw_routes--; });

            if (!statistic.withdraw_routes)
            {
                _withdraw_scorumpower_route_statistic_service.remove(statistic);
            }
        }
        else
        {
            const auto& wvr = _withdraw_scorumpower_route_service.get(from_object.id, to_object.id);

            _withdraw_scorumpower_route_service.update(wvr, [&](withdraw_scorumpower_route_object& wvdo) {
                wvdo.percent = percent;
                wvdo.auto_vest = auto_vest;
            });
        }

        uint16_t total_percent = _withdraw_scorumpower_route_service.total_percent(from_object.id);

        FC_ASSERT(total_percent <= SCORUM_100_PERCENT,
                  "More than 100% of vesting withdrawals allocated to destinations.");
    }

private:
    withdraw_scorumpower_route_service_i& _withdraw_scorumpower_route_service;
    withdraw_scorumpower_route_statistic_service_i& _withdraw_scorumpower_route_statistic_service;
};

// set_withdraw_scorumpower_route_to_account_evaluator

set_withdraw_scorumpower_route_to_account_evaluator::set_withdraw_scorumpower_route_to_account_evaluator(
    data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, set_withdraw_scorumpower_route_to_account_evaluator>(services)
    , _impl(new set_withdraw_scorumpower_route_evaluator_impl(services))
    , _account_service(services.account_service())
{
}

set_withdraw_scorumpower_route_to_account_evaluator::~set_withdraw_scorumpower_route_to_account_evaluator()
{
    // required for _impl correct compilation
}

void set_withdraw_scorumpower_route_to_account_evaluator::do_apply(
    const set_withdraw_scorumpower_route_to_account_evaluator::operation_type& op)
{
    _account_service.check_account_existence(op.from_account);
    _account_service.check_account_existence(op.to_account);

    const auto& from_account = _account_service.get_account(op.from_account);
    const auto& to_account = _account_service.get_account(op.to_account);

    _impl->do_apply(from_account, to_account, op.percent, op.auto_vest);
}

// set_withdraw_scorumpower_route_to_dev_pool_evaluator

set_withdraw_scorumpower_route_to_dev_pool_evaluator::set_withdraw_scorumpower_route_to_dev_pool_evaluator(
    data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, set_withdraw_scorumpower_route_to_dev_pool_evaluator>(services)
    , _impl(new set_withdraw_scorumpower_route_evaluator_impl(services))
    , _account_service(services.account_service())
    , _dev_pool_service(services.dev_pool_service())
{
}

set_withdraw_scorumpower_route_to_dev_pool_evaluator::~set_withdraw_scorumpower_route_to_dev_pool_evaluator()
{
    // required for _impl correct compilation
}

void set_withdraw_scorumpower_route_to_dev_pool_evaluator::do_apply(
    const set_withdraw_scorumpower_route_to_dev_pool_evaluator::operation_type& op)
{
    _account_service.check_account_existence(op.from_account);
    FC_ASSERT(_dev_pool_service.is_exists());

    const auto& from_account = _account_service.get_account(op.from_account);
    const auto& to_pool = _dev_pool_service.get();

    _impl->do_apply(from_account, to_pool, op.percent, op.auto_vest);
}

//

set_withdraw_scorumpower_route_context::set_withdraw_scorumpower_route_context(data_service_factory_i& services,
                                                                       const account_name_type& account,
                                                                       uint16_t percent,
                                                                       bool auto_vest)
    : _services(services)
    , _account(account)
    , _percent(percent)
    , _auto_vest(auto_vest)
{
}

void set_withdraw_scorumpower_route_from_dev_pool_task::on_apply(set_withdraw_scorumpower_route_context& ctx)
{
    set_withdraw_scorumpower_route_evaluator_impl impl(ctx.services());

    dev_pool_service_i& dev_pool_service = ctx.services().dev_pool_service();
    account_service_i& account_service = ctx.services().account_service();

    FC_ASSERT(dev_pool_service.is_exists());

    account_service.check_account_existence(ctx.account());

    const auto& from_pool = dev_pool_service.get();
    const auto& to_account = account_service.get_account(ctx.account());

    impl.do_apply(from_pool, to_account, ctx.percent(), ctx.auto_vest());
}
}
}
