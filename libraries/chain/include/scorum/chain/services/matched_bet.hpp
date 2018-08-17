#pragma once
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct matched_bet_service_i : public base_service_i<matched_bet_object>
{
    virtual const matched_bet_object& get_matched_bets(const matched_bet_id_type&) const = 0;

    using matched_bet_call_type = std::function<void(const matched_bet_service_i::object_type&)>;

    virtual void foreach_bets(const bet_id_type&, matched_bet_call_type) const = 0;
};

class dbs_matched_bet : public dbs_service_base<matched_bet_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_matched_bet(database& db);

public:
    virtual const matched_bet_object& get_matched_bets(const matched_bet_id_type&) const override;

    virtual void foreach_bets(const bet_id_type&, matched_bet_call_type) const override;
};
}
}
