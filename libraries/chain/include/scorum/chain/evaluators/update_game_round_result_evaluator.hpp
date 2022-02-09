#pragma once

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

#include <scorum/chain/dba/db_accessor_fwd.hpp>
#include <scorum/chain/schema/scorum_objects_fwd.hpp>

namespace scorum {
namespace chain {

struct dynamic_global_property_service_i;
struct data_service_factory_i;
struct hardfork_property_service_i;

class update_game_round_result_evaluator : public evaluator_impl<data_service_factory_i, update_game_round_result_evaluator>
{
public:
    using operation_type = scorum::protocol::update_game_round_result_operation;

    update_game_round_result_evaluator(data_service_factory_i&, dba::db_accessor<account_object>&, dba::db_accessor<game_round_object>&);

    void do_apply(const operation_type& op);

private:
    dba::db_accessor<account_object>& _account_dba;
    dba::db_accessor<game_round_object>& _game_round_dba;
    hardfork_property_service_i& _hardfork_service;
};

} // namespace chain
} // namespace scorum
