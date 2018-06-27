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
    _account_service.check_account_existence(op.owner);

#ifndef IS_LOW_MEM
    switch (op.type)
    {
    case budget_type::post:
    {
        _post_budget_service.update(_post_budget_service.get_budget(op.owner, op.budget_id),
                                    [&](post_budget_object& b) { fc::from_string(b.json_metadata, op.json_metadata); });
    }
    break;
    case budget_type::banner:
    {
        _banner_budget_service.update(
            _banner_budget_service.get_budget(op.owner, op.budget_id),
            [&](banner_budget_object& b) { fc::from_string(b.json_metadata, op.json_metadata); });
    }
    break;
    }
#endif
}
}
}
