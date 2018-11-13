#pragma once

#include <scorum/chain/evaluators/evaluator.hpp>

#include <scorum/chain/dba/db_accessor_fwd.hpp>
#include <scorum/chain/schema/scorum_objects_fwd.hpp>

namespace scorum {
namespace chain {

struct data_service_factory_i;
struct betting_matcher_i;
struct betting_service_i;

class post_bet_evaluator : public evaluator_impl<data_service_factory_i, post_bet_evaluator>
{
public:
    using operation_type = scorum::protocol::post_bet_operation;

    post_bet_evaluator(data_service_factory_i&,
                       betting_matcher_i&,
                       betting_service_i&,
                       dba::db_accessor<game_object>&,
                       dba::db_accessor<account_object>&,
                       dba::db_accessor<bet_uuid_history_object>&);

    void do_apply(const operation_type& op);

private:
    betting_matcher_i& _betting_matcher;
    betting_service_i& _betting_svc;

    dba::db_accessor<game_object>& _game_dba;
    dba::db_accessor<account_object>& _account_dba;
    dba::db_accessor<bet_uuid_history_object>& _uuid_hist_dba;
};
}
}
