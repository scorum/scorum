#include <scorum/chain/evaluators/update_budget_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/budget_objects.hpp>

namespace scorum {
namespace chain {

update_budget_evaluator::update_budget_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, update_budget_evaluator>(services)
    , _account_service(services.account_service())
    , _post_budget_service(services.post_budget_service())
    , _banner_budget_service(services.banner_budget_service())
{
}

void update_budget_evaluator::do_apply(const operation_type& op)
{
    switch (op.type)
    {
    case budget_type::post:
        update_budget(_post_budget_service, op);
        break;
    case budget_type::banner:
        update_budget(_banner_budget_service, op);
        break;
    }
}

template <protocol::budget_type budget_type_v>
void update_budget_evaluator::update_budget(adv_budget_service_i<budget_type_v>& budget_svc, const operation_type& op)
{
    _account_service.check_account_existence(op.owner);
    FC_ASSERT(budget_svc.is_exists(op.budget_id), "Budget with id ${id} doesn't exist", ("id", op.budget_id));

    const auto& budget = budget_svc.get(op.budget_id);
    FC_ASSERT(budget.owner == op.owner, "These is not [${o}/${id}] budget", ("o", op.owner)("id", op.budget_id));

#ifndef IS_LOW_MEM
    budget_svc.update(budget, [&](auto& b) { fc::from_string(b.json_metadata, op.json_metadata); });
#else
    boost::ignore_unused_variable_warning(budget);
#endif
}
}
}
