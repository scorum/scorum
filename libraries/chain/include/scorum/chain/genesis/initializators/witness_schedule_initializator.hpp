#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct witness_schedule_initializator_impl : public initializator
{
    virtual initializators get_type() const
    {
        return witness_schedule_initializator;
    }

    virtual initializators_reqired_type get_reqired_types() const
    {
        return { witnesses_initializator };
    }

    virtual void apply(initializator_context&);
};
}
}
}
