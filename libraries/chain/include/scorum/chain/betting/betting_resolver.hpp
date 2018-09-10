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
}
namespace chain {

using scorum::protocol::betting::wincase_type;

struct data_service_factory_i;
struct matched_bet_service_i;
struct bet_service_i;
struct account_service_i;
class game_object;
class bet_object;
class matched_bet_object;
class pending_bet_object;

namespace betting {

struct betting_service_i;

struct betting_resolver_i
{
    virtual void resolve_matched_bets(const chainbase::oid<game_object>& game_id,
                                      const fc::shared_flat_set<wincase_type>& results) const = 0;
};

class betting_resolver : public betting_resolver_i
{
public:
    betting_resolver(betting_service_i& betting_svc,
                     matched_bet_service_i& matched_bet_svc,
                     bet_service_i& bet_svc,
                     account_service_i& account_svc);

    void resolve_matched_bets(const chainbase::oid<game_object>& game_id,
                              const fc::shared_flat_set<wincase_type>& results) const override;

private:
    betting_service_i& _betting_svc;
    matched_bet_service_i& _matched_bet_svc;
    bet_service_i& _bet_svc;
    account_service_i& _account_svc;
};
}
}
}
