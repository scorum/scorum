#pragma once

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

struct account_service_i;
struct game_service_i;
struct betting_matcher_i;
struct pending_bet_service_i;
struct dynamic_global_property_service_i;
struct betting_service_i;

class post_bet_evaluator : public evaluator_impl<data_service_factory_i, post_bet_evaluator>
{
public:
    using operation_type = scorum::protocol::post_bet_operation;

    post_bet_evaluator(data_service_factory_i&, betting_matcher_i&, betting_service_i&);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    game_service_i& _game_service;
    betting_matcher_i& _betting_matcher;
    pending_bet_service_i& _pending_bet_svc;
    dynamic_global_property_service_i& _dgp_svc;
    betting_service_i& _betting_svc;
};
}
}
