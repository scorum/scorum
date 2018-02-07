#pragma once

#include <vector>
#include <map>
#include <memory>

namespace scorum {
namespace chain {

class data_service_factory_i;
class genesis_state_type;

namespace genesis {

enum initializators
{
    accounts_initializator_type = 0,
    founders_initializator_type,
    witnesses_initializator_type,
};

class mark
{
public:
    mark(bool m = false)
        : _m(m)
    {
    }

    operator bool() const
    {
        return _m;
    }

    bool operator!() const
    {
        return !_m;
    }

    bool _m = false;
};

struct initializator
{
    virtual ~initializator()
    {
    }

    virtual initializators get_type() const = 0;

    using initializators_reqired_type = std::vector<initializators>;

    virtual initializators_reqired_type get_reqired_types() const
    {
        return {};
    }

    virtual void apply(data_service_factory_i&, const genesis_state_type&) = 0;
};

class initializators_registry
{
protected:
    explicit initializators_registry(data_service_factory_i& services, const genesis_state_type& genesis_state);

    using initializators_mark_type = std::map<initializators, mark>;
    using initializators_ptr_type = std::map<initializators, std::unique_ptr<initializator>>;

public:
    template <typename InitializatorType> const initializator& register_initializator(InitializatorType* e)
    {
        _initializators_ptr[e->get_type()].reset(e);
        return *e;
    }

    void init(initializators t);
    template <typename InitializatorType> void init(InitializatorType& e)
    {
        init(e.get_type());
    }

private:
    data_service_factory_i& _services;
    const genesis_state_type& _genesis_state;
    initializators_mark_type _initializators_applied;
    initializators_ptr_type _initializators_ptr;
};
}
}
}
