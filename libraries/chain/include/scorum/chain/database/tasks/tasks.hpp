#pragma once

#include <fc/reflect/reflect.hpp>
#include <fc/exception/exception.hpp>

#include <vector>
#include <map>
#include <memory>

namespace scorum {
namespace chain {

class data_service_factory_i;

namespace database_ns {

enum tasks
{
    task_process_funds = 0,
    task_process_comments_cashout,
};

class mark_with_block_num
{
public:
    mark_with_block_num(bool m = false)
        : _m(m)
    {
    }

    mark_with_block_num(uint32_t per_block_num, bool m = false)
        : _per_block_num(per_block_num) // do task per 'per_block_num'
        , _m(m)
    {
        FC_ASSERT(per_block_num > 0);
    }

    // return 'true' if the task execution condition is satisfied
    bool should_process(uint32_t block_num)
    {
        FC_ASSERT(block_num > 0);
        if (!_last_block_num)
            _last_block_num = block_num - 1;
        // if enough blocks are produced
        bool ret = (block_num - _last_block_num > _per_block_num);
        ret |= !_m; // or not already done
        _last_block_num = block_num;
        _m = true;
        return ret;
    }

    operator bool() const
    {
        return _m;
    }

    bool operator!() const
    {
        return !_m;
    }

private:
    uint32_t _per_block_num = 1u;
    uint32_t _last_block_num = 0u;
    bool _m = false;
};

struct task
{
    virtual ~task()
    {
    }

    virtual tasks get_type() const = 0;

    virtual void apply(data_service_factory_i&) = 0;
};

class tasks_registry
{
protected:
    explicit tasks_registry(data_service_factory_i& services);

    using tasks_ptr_type = std::pair<mark_with_block_num, std::unique_ptr<task>>;
    using tasks_mark_type = std::map<tasks, tasks_ptr_type>;

public:
    template <typename TaskType> const task& register_task(TaskType* e, uint32_t per_block_num = 1u)
    {
        FC_ASSERT(e);
        tasks_ptr_type& new_task = _tasks[e->get_type()];
        new_task.first = mark_with_block_num(per_block_num);
        new_task.second.reset(e);
        return *e;
    }

    void process_tasks(uint32_t block_num);
    void process_task(uint32_t block_num, tasks t);
    template <typename TaskType> void init(uint32_t block_num, TaskType& e)
    {
        process_task(block_num, e.get_type());
    }

private:
    void process_task(task&);

    data_service_factory_i& _services;
    tasks_mark_type _tasks;
};
}
}
}

// clang-format off
FC_REFLECT_ENUM(scorum::chain::database_ns::tasks,
                (task_process_funds)
                (task_process_comments_cashout))
// clang-format on
