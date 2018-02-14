#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {
namespace genesis {

using scorum::protocol::account_name_type;
using scorum::protocol::asset;

struct founders_initializator_impl : public initializator
{
    virtual initializators get_type() const
    {
        return founders_initializator;
    }

    virtual initializators_reqired_type get_reqired_types() const
    {
        return { global_property_initializator, accounts_initializator };
    }

    virtual void apply(initializator_context&);

private:
    bool is_founders_pool_exists(initializator_context&);
    void check_founders(initializator_context&);
    asset distribure_sp_by_percent(initializator_context&, account_name_type& pitiful);
    void distribure_sp_rest(initializator_context&, const asset& rest, const account_name_type& pitiful);
    void feed_account(initializator_context& ctx, const account_name_type& name, const asset& sp);
};
}
}
}
