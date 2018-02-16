#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {
namespace genesis {

using scorum::protocol::asset;

struct registration_bonus_initializator_impl : public initializator
{
    virtual void on_apply(initializator_context&);

private:
    asset allocate_cash(initializator_context&);
};
}
}
}
