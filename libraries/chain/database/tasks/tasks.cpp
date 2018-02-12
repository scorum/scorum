#include <scorum/chain/database/tasks/tasks.hpp>

#include <scorum/chain/data_service_factory.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

tasks_registry::tasks_registry(data_service_factory_i& services)
    : _services(services)
{
}

void tasks_registry::process_tasks(uint32_t block_num)
{
    for (auto it = _tasks.begin(); it != _tasks.end(); ++it)
    {
        tasks_ptr_type& _task = it->second;
        if (!_task.first.should_process(block_num))
            continue;
        const auto& ptask = _task.second;
        process_task(*ptask);
    }
}

void tasks_registry::process_task(uint32_t block_num, tasks t)
{
    tasks_mark_type::iterator it = _tasks.find(t);
    FC_ASSERT(_tasks.end() != it, "Task ${1} is not registered.", ("1", t));
    tasks_ptr_type& _task = it->second;
    if (_task.first.should_process(block_num))
    {
        const auto& ptask = _task.second;
        process_task(*ptask);
    }
}

void tasks_registry::process_task(task& task)
{
    auto t = task.get_type();
    dlog("Task ${1} is processing.", ("1", t));

    task.apply(_services);

    dlog("Task ${1} is done.", ("1", t));
}
}
}
}
