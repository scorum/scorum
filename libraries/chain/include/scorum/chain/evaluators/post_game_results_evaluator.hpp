#pragma once
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>
#include <scorum/protocol/betting/wincase.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;

class game_object;

struct account_service_i;
struct betting_service_i;
struct dynamic_global_property_service_i;
struct game_service_i;

class post_game_results_evaluator : public evaluator_impl<data_service_factory_i, post_game_results_evaluator>
{
public:
    using operation_type = post_game_results_operation;

    post_game_results_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);

private:
    void validate_winners(const game_object& game, const fc::flat_set<betting::wincase_type>& winners) const;

private:
    account_service_i& _account_service;
    dynamic_global_property_service_i& _dprops_service;
    betting_service_i& _betting_service;
    game_service_i& _game_service;
};
}
}
