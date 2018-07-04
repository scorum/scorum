#include <scorum/chain/database/block_tasks/process_contracts_expiration.hpp>

#include <scorum/chain/services/atomicswap.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/schema/atomicswap_objects.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_contracts_expiration::on_apply(block_task_context& ctx)
{
    debug_log(ctx.get_block_info(), "process_contracts_expiration BEGIN");

    data_service_factory_i& services = ctx.services();
    atomicswap_service_i& atomicswap_service = services.atomicswap_service();
    dynamic_global_property_service_i& dyn_prop_service = services.dynamic_global_property_service();

    auto contracts = atomicswap_service.get_contracts();
    const auto& props = dyn_prop_service.get();

    for (const atomicswap_contract_object& contract : contracts)
    {
        if (props.time >= contract.deadline)
        {
            if (contract.secret.empty())
            {
                auto owner = contract.owner;
                auto refund_amount = contract.amount;

                // only for initiator or not redeemed participant contracts
                atomicswap_service.refund_contract(contract);

                ctx.push_virtual_operation(expired_contract_refund_operation(owner, refund_amount));
            }
            else
            {
                atomicswap_service.remove(contract);
            }
        }
    }

    debug_log(ctx.get_block_info(), "process_contracts_expiration END");
}
}
}
}
