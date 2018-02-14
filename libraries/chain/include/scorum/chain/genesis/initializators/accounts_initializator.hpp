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

    virtual void apply(initializator_context&);

private:
    void check_accounts_supply(initializator_context&);
};
}
}
}
