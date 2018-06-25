#include <scorum/chain/evaluators/close_budget_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>

#include <scorum/chain/database/budget_management_algorithms.hpp>

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
    , _dprops_service(services.dynamic_global_property_service())
{
}

void close_budget_evaluator::do_apply(const close_budget_evaluator::operation_type& op)
{
    _account_service.check_account_existence(op.owner);

    switch (op.type)
    {
    case budget_type::post:
    {
        const auto& budget = _post_budget_service.get(op.budget_id);
        post_budget_management_algorithm(_post_budget_service, _dprops_service, _account_service).close_budget(budget);
    }
    break;
    case budget_type::banner:
    {
        const auto& budget = _banner_budget_service.get(op.budget_id);
        banner_budget_management_algorithm(_banner_budget_service, _dprops_service, _account_service)
            .close_budget(budget);
    }
    break;
    }
}
}
}
