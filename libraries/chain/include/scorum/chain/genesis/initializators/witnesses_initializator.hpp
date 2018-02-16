#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct witnesses_initializator_impl : public initializator
{
    virtual void on_apply(initializator_context&);
};
}
}
}
