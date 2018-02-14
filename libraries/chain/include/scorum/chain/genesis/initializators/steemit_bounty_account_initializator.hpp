#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct steemit_bounty_account_initializator_impl : public initializator
{
    virtual initializators get_type() const
    {
        return steemit_bounty_account_initializator;
    }

    virtual initializators_reqired_type get_reqired_types() const
    {
        return { global_property_initializator, accounts_initializator };
    }

    virtual void apply(initializator_context&);

private:
    bool is_steemit_pool_exists(initializator_context&);
    void check_accounts(initializator_context& ctx);
};
}
}
}
