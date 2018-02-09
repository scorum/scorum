#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct global_property_initializator_impl : public initializator
{
    virtual initializators get_type() const
    {
        return global_property_initializator;
    }

    virtual void apply(data_service_factory_i& services, const genesis_state_type& genesis_state);
};
}
}
}
