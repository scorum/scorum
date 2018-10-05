#include <scorum/chain/evaluators/close_budget_by_advertising_moderator_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/advertising_property.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/budget_objects.hpp>

namespace scorum {
namespace chain {

close_budget_by_advertising_moderator_evaluator::close_budget_by_advertising_moderator_evaluator(
    data_service_factory_i& services, database_virtual_operations_emmiter_i& virt_op_emmiter)
    : evaluator_impl<data_service_factory_i, close_budget_by_advertising_moderator_evaluator>(services)
    , _account_service(services.account_service())
    , _post_budget_service(services.post_budget_service())
    , _banner_budget_service(services.banner_budget_service())
    , _adv_property_service(services.advertising_property_service())
    , _virt_op_emmiter(virt_op_emmiter)
{
}

void close_budget_by_advertising_moderator_evaluator::do_apply(
    const close_budget_by_advertising_moderator_evaluator::operation_type& op)
{
    switch (op.type)
    {
    case budget_type::post:
        close_budget(_post_budget_service, op);
        break;
    case budget_type::banner:
        close_budget(_banner_budget_service, op);
        break;
    }
}

template <protocol::budget_type budget_type_v>
void close_budget_by_advertising_moderator_evaluator::close_budget(adv_budget_service_i<budget_type_v>& budget_svc,
                                                                   const operation_type& op)
{
    _account_service.check_account_existence(op.moderator);

    auto& current_moderator = _adv_property_service.get().moderator;
    FC_ASSERT(current_moderator != SCORUM_MISSING_MODERATOR_ACCOUNT, "Advertising moderator was not set");
    FC_ASSERT(current_moderator == op.moderator, "User ${1} is not the advertising moderator", ("1", op.moderator));
    FC_ASSERT(budget_svc.is_exists(op.budget_id), "Budget with id ${id} doesn't exist", ("id", op.budget_id));

    const auto& budget = budget_svc.get(op.budget_id);
    auto balance_rest = budget.balance;

    budget_svc.finish_budget(op.budget_id);

    _virt_op_emmiter.push_virtual_operation(
        budget_closing_operation(budget_type_v, budget.owner, op.budget_id, balance_rest));
}
}
}
