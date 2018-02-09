#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct accounts_initializator_impl : public initializator
{
    virtual initializators get_type() const
    {
        return accounts_initializator;
    }

    virtual initializators_reqired_type get_reqired_types() const
    {
        return { global_property_initializator };
    }

    virtual void apply(data_service_factory_i& services, const genesis_state_type& genesis_state);
};
}
}
}