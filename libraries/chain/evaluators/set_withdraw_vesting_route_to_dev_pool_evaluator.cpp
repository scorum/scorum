#include <scorum/chain/evaluators/set_withdraw_vesting_route_to_dev_pool_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>

#include <scorum/chain/schema/account_objects.hpp>

namespace scorum {
namespace chain {

set_withdraw_vesting_route_to_dev_pool_evaluator::set_withdraw_vesting_route_to_dev_pool_evaluator(
    data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, set_withdraw_vesting_route_to_dev_pool_evaluator>(services)
    , _account_service(db().account_service())
    , _withdraw_service(db().withdraw_vesting_route_service())
{
}

void set_withdraw_vesting_route_to_dev_pool_evaluator::do_apply(
    const set_withdraw_vesting_route_to_dev_pool_evaluator::operation_type& op)
{
    _account_service.check_account_existence(op.from_account);

    // TODO
}
}
}
