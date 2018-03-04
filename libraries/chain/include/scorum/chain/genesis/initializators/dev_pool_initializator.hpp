#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {

class account_object;

using scorum::protocol::asset;
using scorum::protocol::account_name_type;

namespace genesis {

struct dev_pool_initializator_impl : public initializator
{
    virtual void on_apply(initializator_context&);

private:
    bool is_zero_supply(initializator_context&);
    void create_dev_committee_and_set_pool_balance(initializator_context&);
    void increase_total_supply(initializator_context&);
};
}
}
}
