#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

struct account_service_i;
namespace betting {
struct betting_service_i;
}

class cancel_bet_evaluator : public evaluator_impl<data_service_factory_i, cancel_bet_evaluator>
{
public:
    using operation_type = scorum::protocol::cancel_bet_operation;

    cancel_bet_evaluator(data_service_factory_i& services, betting::betting_service_i&);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    betting::betting_service_i& _betting_service;
};
}
}
