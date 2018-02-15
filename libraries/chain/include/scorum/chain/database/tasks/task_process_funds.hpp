#pragma once

#include <scorum/chain/database/tasks/tasks.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

struct task_process_funds_impl : public task
{
    virtual tasks get_type() const
    {
        return task_process_funds;
    }

    virtual void apply(task_context&);
};
}
}
}
