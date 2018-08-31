#pragma once
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

struct account_service_i;
struct game_service_i;
namespace betting {
struct betting_service_i;
struct betting_resolver_i;
}

class cancel_game_evaluator : public evaluator_impl<data_service_factory_i, cancel_game_evaluator>
{
public:
    using operation_type = scorum::protocol::cancel_game_operation;

    cancel_game_evaluator(data_service_factory_i& services, betting::betting_service_i&, betting::betting_resolver_i&);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    chain::betting::betting_service_i& _betting_service;
    chain::betting::betting_resolver_i& _betting_resolver;
    game_service_i& _game_service;
};
}
}
