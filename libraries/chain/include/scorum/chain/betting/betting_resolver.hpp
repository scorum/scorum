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
class game_object;
class matched_bet_object;
struct bet_data;

struct betting_service_i;

struct betting_resolver_i
{
    virtual void resolve_matched_bets(const chainbase::oid<game_object>& game_id,
                                      const fc::shared_flat_set<protocol::wincase_type>& results) const = 0;
};

class betting_resolver : public betting_resolver_i
{
public:
    betting_resolver(betting_service_i& betting_svc,
                     account_service_i& account_svc,
                     database_virtual_operations_emmiter_i& virt_op_emitter,
                     dba::db_accessor<matched_bet_object>& matched_bet_dba,
                     dba::db_accessor<game_object>& game_dba);

    void resolve_matched_bets(const chainbase::oid<game_object>& game_id,
                              const fc::shared_flat_set<protocol::wincase_type>& results) const override;

private:
    betting_service_i& _betting_svc;
    account_service_i& _account_svc;
    database_virtual_operations_emmiter_i& _virt_op_emitter;
    dba::db_accessor<matched_bet_object>& _matched_bet_dba;
    dba::db_accessor<game_object>& _game_dba;
};
}
}
