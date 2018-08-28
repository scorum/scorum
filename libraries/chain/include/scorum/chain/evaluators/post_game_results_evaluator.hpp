#pragma once
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>
#include <scorum/protocol/betting/wincase.hpp>
#include <fc/shared_containers.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;
using scorum::protocol::betting::market_type;
using scorum::protocol::betting::wincase_type;

class game_object;

struct account_service_i;
struct dynamic_global_property_service_i;
namespace betting {
struct betting_service_i;
}
struct game_service_i;

class post_game_results_evaluator : public evaluator_impl<data_service_factory_i, post_game_results_evaluator>
{
public:
    using operation_type = post_game_results_operation;

    post_game_results_evaluator(data_service_factory_i& services, betting::betting_service_i& betting_service);

    void do_apply(const operation_type& op);

private:
    void validate_all_winners_present(const fc::shared_flat_set<market_type>& markets,
                                      const fc::flat_set<wincase_type>& winners) const;
    void validate_opposite_winners_absent(const fc::flat_set<wincase_type>& winners) const;

private:
    account_service_i& _account_service;
    betting::betting_service_i& _betting_service;
    game_service_i& _game_service;
    dynamic_global_property_service_i& _dprops_service;
};
}
}
