#pragma once

#include <vector>
#include <functional>
#include <boost/type_index.hpp>
#include <fc/log/logger.hpp>

namespace scorum {
namespace chain {

template <typename ContextType> struct task_reentrance_guard_i
{
    virtual ~task_reentrance_guard_i()
    {
    }

    virtual bool is_allowed(ContextType& ctx) = 0;
    virtual void apply(ContextType& ctx) = 0;
};

template <typename ContextType> class dummy_reentrance_guard : public task_reentrance_guard_i<ContextType>
{
public:
    virtual bool is_allowed(ContextType&)
    {
        return true;
    }
    virtual void apply(ContextType&)
    {
    }
};

class data_service_factory_i;

template <typename ContextType = data_service_factory_i,
          typename ReentranceGuardType = dummy_reentrance_guard<ContextType>>
class task
{
public:
    virtual ~task()
    {
    }

    task& after(task& impl)
    {
        _after.push_back(std::ref(impl));
        return *this;
    }
    task& before(task& impl)
    {
        _before.push_back(std::ref(impl));
        return *this;
    }

    void apply(ContextType& ctx)
    {
        if (!_guard.is_allowed(ctx))
            return;
        _guard.apply(ctx);

        for (task& r : _after)
        {
            r.apply(ctx);
        }

        on_apply(ctx);

        for (task& r : _before)
        {
            r.apply(ctx);
        }
    }

protected:
    virtual void on_apply(ContextType&) = 0;

private:
    using tasks_reqired_type = std::vector<std::reference_wrapper<task>>;

    tasks_reqired_type _after;
    tasks_reqired_type _before;
    ReentranceGuardType _guard;
};
}
}
