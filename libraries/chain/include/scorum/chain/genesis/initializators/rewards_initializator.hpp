#pragma once

#include <scorum/chain/genesis/initializators/initializators.hpp>

namespace scorum {
namespace chain {
namespace genesis {

struct rewards_initializator_impl : public initializator
{
    virtual void on_apply(initializator_context&);

private:
    void create_scr_reward_fund(initializator_context&);
    void create_sp_reward_fund(initializator_context&);
    void create_balancers(initializator_context&);
    void create_fund_budget(initializator_context&);
};
}
}
}
