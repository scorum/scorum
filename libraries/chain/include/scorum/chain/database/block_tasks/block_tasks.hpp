#pragma once

#include <scorum/chain/tasks_base.hpp>

#include <scorum/chain/database/database_virtual_operations.hpp>

namespace scorum {
namespace chain {

namespace database_ns {

struct block_task_context : public database_virtual_operations_emmiter_i
{
    explicit block_task_context(data_service_factory_i& services,
                                database_virtual_operations_emmiter_i& vops,
                                uint32_t block_num);

    virtual void push_virtual_operation(const operation& op);

    data_service_factory_i& services;
    uint32_t block_num;

private:
    database_virtual_operations_emmiter_i& _vops;
};

class per_block_num_apply_censor : public task_censor_i<block_task_context>
{
public:
    explicit per_block_num_apply_censor(uint32_t per_block_num);

    virtual bool is_allowed(block_task_context& ctx)
    {
        return ctx.block_num - _last_block_num >= _per_block_num;
    }
    virtual void apply(block_task_context& ctx)
    {
        _last_block_num = ctx.block_num;
    }

private:
    uint32_t _per_block_num;
    uint32_t _last_block_num = 0u;
};

class block_task : public task<block_task_context>
{
public:
    block_task(uint32_t per_block_num = 1u) // by default, apply for each block
        : _censor(per_block_num)
    {
        set_censor(&_censor);
    }

private:
    per_block_num_apply_censor _censor;
};

} // database_ns
}
}
