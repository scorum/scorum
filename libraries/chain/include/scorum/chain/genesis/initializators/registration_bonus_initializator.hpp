#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {
namespace genesis {

using scorum::protocol::asset;

struct registration_bonus_initializator_impl : public initializator
{
    virtual initializators get_type() const
    {
        return registration_bonus_initializator;
    }

    virtual initializators_reqired_type get_reqired_types() const
    {
        return { accounts_initializator, registration_initializator };
    }

    virtual void apply(data_service_factory_i& services, const genesis_state_type& genesis_state);

private:
    asset allocate_cash(data_service_factory_i& services);
};
}
}
}
