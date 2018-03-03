#pragma once

#include <scorum/chain/tasks_base.hpp>

#include <scorum/chain/database/database_virtual_operations.hpp>

#include <fc/exception/exception.hpp>

namespace scorum {
namespace chain {

namespace database_ns {

class block_task_context : public database_virtual_operations_emmiter_i
{
public:
    explicit block_task_context(data_service_factory_i& services,
                                database_virtual_operations_emmiter_i& vops,
                                uint32_t block_num);

    virtual void push_virtual_operation(const operation& op);

    data_service_factory_i& services() const
    {
        return _services;
    }

    uint32_t block_num() const
    {
        return _block_num;
    }

private:
    data_service_factory_i& _services;
    database_virtual_operations_emmiter_i& _vops;
    uint32_t _block_num;
};

template <uint32_t per_block_num> class per_block_num_apply_guard : public task_reentrance_guard_i<block_task_context>
{
    static_assert(per_block_num > 0u, "Invalid value for per_block_num.");

public:
    virtual bool is_allowed(block_task_context& ctx)
    {
        return ctx.block_num() - _last_block_num >= _per_block_num;
    }
    virtual void apply(block_task_context& ctx)
    {
        _last_block_num = ctx.block_num();
    }

private:
    uint32_t _per_block_num = per_block_num;
    uint32_t _last_block_num = 0u;
};

template <uint32_t per_block_num>
class block_task_type : public task<block_task_context, per_block_num_apply_guard<per_block_num>>
{
protected:
    block_task_type()
    {
    }
};

// By default guard (per_block_num_apply_guard) controls applying one time per one block.
// Use class inherited from block_task in this case.
using block_task = block_task_type<1u>;
// If it is required applying one time per N block, use class inherited from block_task_type<N>

} // database_ns
}
}
