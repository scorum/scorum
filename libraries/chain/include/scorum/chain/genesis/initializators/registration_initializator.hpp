#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct registration_initializator_impl : public initializator
{
    virtual void on_apply(initializator_context&);

private:
    bool is_registration_pool_exists(initializator_context&);
};
}
}
}
