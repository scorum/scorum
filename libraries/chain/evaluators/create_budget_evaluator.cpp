#include <scorum/chain/evaluators/create_budget_evaluator.hpp>

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

create_budget_evaluator::create_budget_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, create_budget_evaluator>(services)
    , _account_service(services.account_service())
    , _post_budget_service(services.post_budget_service())
    , _banner_budget_service(services.banner_budget_service())
    , _dprops_service(services.dynamic_global_property_service())
{
}

void create_budget_evaluator::do_apply(const create_budget_evaluator::operation_type& op)
{
    _account_service.check_account_existence(op.owner);

    const auto& owner = _account_service.get_account(op.owner);

    FC_ASSERT(owner.name != SCORUM_ROOT_POST_PARENT_ACCOUNT, "'${1}' name is not allowed for ordinary budget.",
              ("1", SCORUM_ROOT_POST_PARENT_ACCOUNT));
    FC_ASSERT(owner.balance >= op.balance, "Insufficient funds.",
              ("owner.balance", owner.balance)("balance", op.balance));
    FC_ASSERT(_post_budget_service.get_budgets(owner.name).size()
                      + _banner_budget_service.get_budgets(owner.name).size()
                  < (uint32_t)SCORUM_BUDGETS_LIMIT_PER_OWNER,
              "Can't create more then ${1} budgets per owner.", ("1", SCORUM_BUDGETS_LIMIT_PER_OWNER));

    const auto& dprops = _dprops_service.get();
    time_point_sec start_date = dprops.time;

    switch (op.type)
    {
    case budget_type::post:
    {
        post_budget_management_algorithm(_post_budget_service, _dprops_service, _account_service)
            .create_budget(owner.name, op.balance, start_date, op.deadline, op.permlink);
    }
    break;
    case budget_type::banner:
    {
        banner_budget_management_algorithm(_banner_budget_service, _dprops_service, _account_service)
            .create_budget(owner.name, op.balance, start_date, op.deadline, op.permlink);
    }
    break;
    default:
        FC_ASSERT("Unknown budget type");
    }
}
}
}
