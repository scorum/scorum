#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct dev_committee_initializator_impl : public initializator
{
    virtual void on_apply(initializator_context&);
};

} // namespace genesis
} // namespace scorum
} // namespace chain
