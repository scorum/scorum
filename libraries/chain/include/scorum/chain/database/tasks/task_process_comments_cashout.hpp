#pragma once

#include <scorum/chain/database/tasks/tasks.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

struct task_process_comments_cashout_impl : public task
{
    virtual tasks get_type() const
    {
        return task_process_comments_cashout;
    }

    virtual void apply(data_service_factory_i& services, database_virtual_operations_emmiter_i& vops);
};
}
}
}
