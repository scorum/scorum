#pragma once

#include <scorum/chain/tasks_base.hpp>

#include <scorum/chain/database/database_virtual_operations.hpp>
#include <scorum/chain/database/debug_log.hpp>
#include <fc/exception/exception.hpp>
#include <scorum/protocol/block.hpp>

namespace scorum {
namespace chain {

namespace database_ns {

class block_task_context : public database_virtual_operations_emmiter_i
{
public:
    block_task_context(data_service_factory_i& services,
                       database_virtual_operations_emmiter_i& vops,
                       uint32_t block_num,
                       block_info& block_info);

    virtual void push_virtual_operation(const operation& op);

    data_service_factory_i& services() const
    {
        return _services;
    }

    block_info& get_block_info() const
    {
        return _block_info;
    }

    uint32_t block_num() const
    {
        return _block_num;
    }

private:
    data_service_factory_i& _services;
    database_virtual_operations_emmiter_i& _vops;
    uint32_t _block_num;
    block_info& _block_info;
};

class block_task_type : public task<block_task_context>
{
protected:
    block_task_type()
    {
    }
};

using block_task = block_task_type;

} // database_ns
}
}
