#pragma once

#include <scorum/chain/tasks_base.hpp>

namespace scorum {
namespace chain {
namespace dba {
struct db_accessor_factory;
}
struct data_service_factory_i;
struct genesis_state_type;

namespace genesis {

class initializator_context
{
public:
    explicit initializator_context(data_service_factory_i& services,
                                   dba::db_accessor_factory& dba_factory,
                                   const genesis_state_type& genesis_state);

    data_service_factory_i& services() const
    {
        return _services;
    }

    dba::db_accessor_factory& dba_factory() const
    {
        return _dba_factory;
    }

    const genesis_state_type& genesis_state() const
    {
        return _genesis_state;
    }

private:
    data_service_factory_i& _services;
    dba::db_accessor_factory& _dba_factory;
    const genesis_state_type& _genesis_state;
};

class single_time_apply_guard : public task_reentrance_guard_i<initializator_context>
{
public:
    virtual bool is_allowed(initializator_context&)
    {
        return !_applied;
    }
    virtual void apply(initializator_context&)
    {
        _applied = true;
    }

private:
    bool _applied = false;
};

class initializator : public task<initializator_context, single_time_apply_guard>
{
protected:
    initializator()
    {
    }
};

} // genesis
}
}
