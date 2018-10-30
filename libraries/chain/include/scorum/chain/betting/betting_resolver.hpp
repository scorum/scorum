#pragma once
#include <fc/shared_containers.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/types.hpp>

namespace chainbase {
template <typename TObject> class oid;
}
namespace scorum {
namespace protocol {
struct asset;
enum class bet_resolve_kind : uint8_t;
}
namespace chain {

namespace dba {
template <typename> class db_accessor;
}
struct data_service_factory_i;
struct account_service_i;
struct database_virtual_operations_emmiter_i;

class dynamic_global_property_object;
class game_object;
class matched_bet_object;
struct bet_data;

struct betting_resolver_i
{
    virtual void resolve_matched_bets(uuid_type game_uuid,
                                      const fc::flat_set<protocol::wincase_type>& results) const = 0;
};

class betting_resolver : public betting_resolver_i
{
public:
    betting_resolver(account_service_i&,
                     database_virtual_operations_emmiter_i&,
                     dba::db_accessor<matched_bet_object>&,
                     dba::db_accessor<game_object>&,
                     dba::db_accessor<dynamic_global_property_object>&);

    void resolve_matched_bets(uuid_type game_uuid, const fc::flat_set<protocol::wincase_type>& results) const override;

private:
    account_service_i& _account_svc;
    database_virtual_operations_emmiter_i& _virt_op_emitter;
    dba::db_accessor<matched_bet_object>& _matched_bet_dba;
    dba::db_accessor<game_object>& _game_dba;
    dba::db_accessor<dynamic_global_property_object>& _dprop_dba;
};
}
}
