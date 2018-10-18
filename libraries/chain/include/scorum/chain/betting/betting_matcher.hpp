#pragma once
#include <scorum/chain/betting/betting_math.hpp>

namespace scorum {
namespace chain {

class pending_bet_object;
class game_object;

struct data_service_factory_i;

struct dynamic_global_property_service_i;
struct betting_property_service_i;
struct betting_service_i;
struct pending_bet_service_i;
struct matched_bet_service_i;
struct database_virtual_operations_emmiter_i;

namespace dba {
template <typename> class db_accessor;
}

struct betting_matcher_i
{
    virtual void match(const pending_bet_object& bet1) = 0;
};

class betting_matcher : public betting_matcher_i
{
public:
    betting_matcher(data_service_factory_i&,
                    database_virtual_operations_emmiter_i&,
                    betting_service_i&,
                    dba::db_accessor<game_object>&);

    void match(const pending_bet_object& bet2) override;

private:
    bool is_bets_matched(const pending_bet_object& bet1, const pending_bet_object& bet2) const;
    bool can_be_matched(const asset& stake, const odds& bet_odds) const;

    betting_service_i& _betting_svc;
    dynamic_global_property_service_i& _dgp_property;
    betting_property_service_i& _betting_property;
    pending_bet_service_i& _pending_bet_service;
    matched_bet_service_i& _matched_bet_service;
    dba::db_accessor<game_object>& _game_dba;
    database_virtual_operations_emmiter_i& _virt_op_emitter;
};
}
}
