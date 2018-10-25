#include <scorum/chain/evaluators/close_budget_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/budget_objects.hpp>

namespace scorum {
namespace chain {

close_budget_evaluator::close_budget_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, close_budget_evaluator>(services)
    , _account_service(services.account_service())
    , _post_budget_service(services.post_budget_service())
    , _banner_budget_service(services.banner_budget_service())
{
}

void close_budget_evaluator::do_apply(const close_budget_evaluator::operation_type& op)
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
void close_budget_evaluator::close_budget(adv_budget_service_i<budget_type_v>& budget_svc, const operation_type& op)
{
    _account_service.check_account_existence(op.owner);
    FC_ASSERT(budget_svc.is_exists(op.uuid), "Budget with uuid ${id} doesn't exist", ("id", op.uuid));

    const auto& budget = budget_svc.get(op.uuid);
    FC_ASSERT(budget.owner == op.owner, "These is not [${o}/${id}] budget", ("o", op.owner)("id", op.uuid));

    budget_svc.finish_budget(op.uuid);
}
}
}
