#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {

class account_object;
class dev_committee_object;

using scorum::protocol::asset;

namespace genesis {

struct dev_pool_initializator_impl : public initializator
{
    virtual void on_apply(initializator_context&);

private:
    bool is_dev_pool_exists(initializator_context&);
    void increase_total_supply(initializator_context& ctx, const asset& sp);
    const account_object& create_locked_account(initializator_context& ctx, const asset& sp);
    const dev_committee_object& create_dev_pool(initializator_context& ctx);
};
}
}
}
