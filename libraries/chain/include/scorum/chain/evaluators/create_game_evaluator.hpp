#pragma once
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

#include <scorum/chain/dba/db_accessor_fwd.hpp>
#include <scorum/chain/schema/scorum_objects_fwd.hpp>

namespace scorum {
namespace chain {

struct account_service_i;
struct dynamic_global_property_service_i;
struct game_service_i;
struct betting_service_i;

class create_game_evaluator : public evaluator_impl<data_service_factory_i, create_game_evaluator>
{
public:
    using operation_type = scorum::protocol::create_game_operation;

    create_game_evaluator(data_service_factory_i&, betting_service_i&, dba::db_accessor<game_uuid_history_object>&);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    dynamic_global_property_service_i& _dprops_service;
    betting_service_i& _betting_service;
    game_service_i& _game_svc;
    dba::db_accessor<game_uuid_history_object>& _uuid_hist_dba;
};
}
}
