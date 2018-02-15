#pragma once

#include <vector>
#include <functional>

namespace scorum {
namespace chain {

class data_service_factory_i;
class genesis_state_type;

namespace genesis {

struct initializator_context
{
    explicit initializator_context(data_service_factory_i& services, const genesis_state_type& genesis_state);

    data_service_factory_i& services;
    const genesis_state_type& genesis_state;
};

class initializator
{
public:
    virtual ~initializator()
    {
    }

    initializator& after(initializator&);

    void apply(initializator_context&);

protected:
    virtual void on_apply(initializator_context&) = 0;

private:
    using initializators_reqired_type = std::vector<std::reference_wrapper<initializator>>;

    initializators_reqired_type _after;
    bool _applied = false;
};
}
}
}
