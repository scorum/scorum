#pragma once

#include <vector>
#include <functional>
#include <boost/type_index.hpp>
#include <fc/log/logger.hpp>

namespace scorum {
namespace chain {

template <typename ContextType> struct task_censor_i
{
    virtual ~task_censor_i()
    {
    }

    virtual bool is_allowed(ContextType& ctx) = 0;
    virtual void apply(ContextType& ctx) = 0;
};

class data_service_factory_i;

template <typename ContextType = data_service_factory_i> class task
{
public:
    virtual ~task()
    {
    }

    using censor_type = task_censor_i<ContextType>;

    template <typename CensorType> void set_censor(CensorType* pcensor = nullptr)
    {
        _pcensor = static_cast<censor_type*>(pcensor);
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
        if (_pcensor)
        {
            if (!_pcensor->is_allowed(ctx))
                return;
            _pcensor->apply(ctx);
        }

        for (task& r : _after)
        {
            r.apply(ctx);
        }

        auto impl = boost::typeindex::type_id_runtime(*this).pretty_name();

        dlog("Task ${impl} is processing.", ("impl", impl));

        on_apply(ctx);

        dlog("Task ${impl} is done.", ("impl", impl));

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
    censor_type* _pcensor = nullptr;
};
}
}
