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

    virtual void apply(initializator_context&);
};
}
}
}
