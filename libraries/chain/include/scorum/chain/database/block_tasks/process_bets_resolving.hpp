#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace chain {
class game_object;
class dynamic_global_property_object;

struct betting_service_i;
struct betting_resolver_i;
struct database_virtual_operations_emmiter_i;
namespace dba {
template <typename> class db_accessor;
}
namespace database_ns {

struct process_bets_resolving : public block_task
{
    process_bets_resolving(betting_service_i&,
                           betting_resolver_i&,
                           database_virtual_operations_emmiter_i&,
                           dba::db_accessor<game_object>&,
                           dba::db_accessor<dynamic_global_property_object>&);

    virtual void on_apply(block_task_context&);

private:
    betting_service_i& _betting_svc;
    betting_resolver_i& _resolver;
    database_virtual_operations_emmiter_i& _vop_emitter;
    dba::db_accessor<game_object>& _game_dba;
    dba::db_accessor<dynamic_global_property_object>& _dprop_dba;
};
}
}
}
