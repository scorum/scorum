#include <scorum/chain/evaluators/set_withdraw_vesting_route_to_dev_pool_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/withdraw_vesting_route_object.hpp>

namespace scorum {
namespace chain {

set_withdraw_vesting_route_to_dev_pool_evaluator::set_withdraw_vesting_route_to_dev_pool_evaluator(
    data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, set_withdraw_vesting_route_to_dev_pool_evaluator>(services)
    , _account_service(db().account_service())
    , _dev_pool_service(db().dev_pool_service())
    , _withdraw_vesting_route_service(db().withdraw_vesting_route_service())
{
}

void set_withdraw_vesting_route_to_dev_pool_evaluator::do_apply(
    const set_withdraw_vesting_route_to_dev_pool_evaluator::operation_type& op)
{
    FC_ASSERT(_dev_pool_service.is_exists());

    const auto& from_account = _account_service.get_account(op.from_account);
    const auto& to_pool = _dev_pool_service.get();

    if (!_withdraw_vesting_route_service.is_exists(from_account.id, to_pool.id))
    {
        FC_ASSERT(op.percent != 0, "Cannot create a 0% destination.");
        FC_ASSERT(from_account.withdraw_routes < SCORUM_MAX_WITHDRAW_ROUTES,
                  "Account already has the maximum number of routes.");

        _withdraw_vesting_route_service.create([&](withdraw_vesting_route_object& wvdo) {
            wvdo.from_account = from_account.id;
            wvdo.to_id = withdraw_route_service::get_to_id(to_pool);
            wvdo.to_object = to_pool.id;
            wvdo.percent = op.percent;
        });

        _account_service.increase_withdraw_routes(from_account);
    }
    else if (op.percent == 0)
    {
        const auto& wvr = _withdraw_vesting_route_service.get(from_account.id, to_pool.id);
        _withdraw_vesting_route_service.remove(wvr);

        _account_service.decrease_withdraw_routes(from_account);
    }
    else
    {
        const auto& wvr = _withdraw_vesting_route_service.get(from_account.id, to_pool.id);

        _withdraw_vesting_route_service.update(wvr, [&](withdraw_vesting_route_object& wvdo) {
            wvdo.from_account = from_account.id;
            wvdo.to_id = withdraw_route_service::get_to_id(to_pool);
            wvdo.to_object = to_pool.id;
            wvdo.percent = op.percent;
        });
    }

    uint16_t total_percent = _withdraw_vesting_route_service.total_percent(from_account.id);

    FC_ASSERT(total_percent <= SCORUM_100_PERCENT, "More than 100% of vesting withdrawals allocated to destinations.");
}
}
}
