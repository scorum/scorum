#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct registration_initializator_impl : public initializator
{
    virtual initializators get_type() const
    {
        return registration_initializator;
    }

    virtual initializators_reqired_type get_reqired_types() const
    {
        return { accounts_initializator };
    }

    virtual void apply(initializator_context&);

private:
    bool is_registration_pool_exists(initializator_context&);
};
}
}
}
