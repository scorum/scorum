#include <scorum/chain/database/block_tasks/block_tasks.hpp>

#include <scorum/chain/data_service_factory.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

block_task_context::block_task_context(data_service_factory_i& _services,
                                       database_virtual_operations_emmiter_i& vops,
                                       uint32_t _block_num)
    : services(_services)
    , block_num(_block_num)
    , _vops(vops)
{
    FC_ASSERT(block_num > 0u);
}

void block_task_context::push_virtual_operation(const operation& op)
{
    _vops.push_virtual_operation(op);
}

per_block_num_apply_censor::per_block_num_apply_censor(uint32_t per_block_num)
    : _per_block_num(per_block_num)
{
    FC_ASSERT(_per_block_num > 0u);
}

} // database_ns
}
}
