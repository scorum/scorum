#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

struct account_service_i;
struct game_service_i;
namespace betting {
struct betting_service_i;
struct betting_matcher_i;
}

class post_bet_evaluator : public evaluator_impl<data_service_factory_i, post_bet_evaluator>
{
public:
    using operation_type = scorum::protocol::post_bet_operation;

    post_bet_evaluator(data_service_factory_i& services, betting::betting_service_i&, betting::betting_matcher_i&);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    game_service_i& _game_service;
    betting::betting_service_i& _betting_service;
    betting::betting_matcher_i& _betting_matcher;
};
}
}
