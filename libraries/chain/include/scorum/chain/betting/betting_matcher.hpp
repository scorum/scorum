#pragma once

#include <scorum/chain/betting/betting_math.hpp>

#include <scorum/chain/schema/bet_objects.hpp>

namespace scorum {
namespace chain {

struct data_service_factory_i;

struct dynamic_global_property_service_i;
struct betting_property_service_i;
struct bet_service_i;
struct pending_bet_service_i;
struct matched_bet_service_i;
enum class pending_bet_kind : uint8_t;

namespace betting {

struct betting_matcher_i
{
    virtual void match(const bet_object& bet, pending_bet_kind bet_kind) = 0;
};

class betting_matcher : public betting_matcher_i
{
public:
    betting_matcher(data_service_factory_i&);

    virtual void match(const bet_object& bet, pending_bet_kind bet_kind) override;

private:
    bool is_bets_matched(const bet_object& bet1, const bet_object& bet2) const;
    bool is_need_matching(const bet_object& bet) const;

    dynamic_global_property_service_i& _dgp_property;
    betting_property_service_i& _betting_property;
    bet_service_i& _bet_service;
    pending_bet_service_i& _pending_bet_service;
    matched_bet_service_i& _matched_bet_service;
};
}
}
}
