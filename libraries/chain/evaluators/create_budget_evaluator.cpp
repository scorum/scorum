#include <scorum/chain/evaluators/create_budget_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/budget_objects.hpp>

#include <boost/uuid/uuid_io.hpp>

namespace scorum {
namespace chain {

create_budget_evaluator::create_budget_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, create_budget_evaluator>(services)
    , _account_svc(services.account_service())
    , _post_budget_svc(services.post_budget_service())
    , _banner_budget_svc(services.banner_budget_service())
    , _dprops_svc(services.dynamic_global_property_service())
{
}

void create_budget_evaluator::do_apply(const create_budget_evaluator::operation_type& op)
{
    _account_svc.check_account_existence(op.owner);

    const auto& owner = _account_svc.get_account(op.owner);

    FC_ASSERT(owner.balance >= op.balance, "Insufficient funds.",
              ("owner.balance", owner.balance)("balance", op.balance));
    FC_ASSERT(_post_budget_svc.get_budgets(owner.name).size() + _banner_budget_svc.get_budgets(owner.name).size()
                  < (uint32_t)SCORUM_BUDGETS_LIMIT_PER_OWNER,
              "Can't create more then ${1} budgets per owner.", ("1", SCORUM_BUDGETS_LIMIT_PER_OWNER));

    FC_ASSERT(op.start <= op.deadline, "Deadline time must be greater or equal than start time");
    FC_ASSERT(op.start > _dprops_svc.head_block_time(), "Start time must be greater than head block time");

    switch (op.type)
    {
    case budget_type::post:
        create_budget(_post_budget_svc, op, owner.name);
        break;
    case budget_type::banner:
        create_budget(_banner_budget_svc, op, owner.name);
        break;
    }
}

template <budget_type budget_type_v>
void create_budget_evaluator::create_budget(adv_budget_service_i<budget_type_v>& budget_svc,
                                            const create_budget_evaluator::operation_type& op,
                                            account_name_type owner)
{
    using namespace boost::uuids;

    FC_ASSERT(!budget_svc.is_exists(op.uuid), "Budget with uuid ${1} already exists", ("1", to_string(op.uuid)));

    budget_svc.create_budget(op.uuid, owner, op.balance, op.start, op.deadline, op.json_metadata);
}
}
}
