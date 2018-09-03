#pragma once
#include <fc/shared_containers.hpp>
#include <scorum/protocol/betting/wincase.hpp>
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
using scorum::protocol::betting::wincase_pair;

struct data_service_factory_i;
struct pending_bet_service_i;
struct matched_bet_service_i;
struct bet_service_i;
struct account_service_i;
class game_object;
class bet_object;
class matched_bet_object;
class pending_bet_object;

namespace betting {

struct betting_resolver_i
{
    virtual void resolve_matched_bets(const chainbase::oid<game_object>& game_id,
                                      const fc::shared_flat_set<wincase_type>& results) const = 0;

    virtual void return_pending_bets(const chainbase::oid<game_object>& game_id) const = 0;
    virtual void return_matched_bets(const chainbase::oid<game_object>& game_id) const = 0;

    virtual void return_bets(const std::vector<std::reference_wrapper<const bet_object>>& bets) const = 0;
};

class betting_resolver : public betting_resolver_i
{
public:
    betting_resolver(pending_bet_service_i& pending_bet_svc,
                     matched_bet_service_i& matched_bet_svc,
                     bet_service_i& bet_svc,
                     account_service_i& account_svc);

    void resolve_matched_bets(const chainbase::oid<game_object>& game_id,
                              const fc::shared_flat_set<wincase_type>& results) const override;

    void return_pending_bets(const chainbase::oid<game_object>& game_id) const override;
    void return_matched_bets(const chainbase::oid<game_object>& game_id) const override;

    // TODO: signature will be changed with new db_accessors mechanism
    void return_bets(const std::vector<std::reference_wrapper<const bet_object>>& bets) const override;

private:
    void increase_balance(const protocol::account_name_type& acc_name, const protocol::asset& stake) const;

private:
    pending_bet_service_i& _pending_bet_svc;
    matched_bet_service_i& _matched_bet_svc;
    bet_service_i& _bet_svc;
    account_service_i& _account_svc;
};
}
}
}
