#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct founders_initializator : public initializator
{
    virtual initializators get_type() const
    {
        return founders_initializator_type;
    }

    virtual initializators_reqired_type get_reqired_types() const
    {
        return { accounts_initializator_type };
    }

    virtual void apply(data_service_factory_i& services, const genesis_state_type& genesis_state);
};
}
}
}
