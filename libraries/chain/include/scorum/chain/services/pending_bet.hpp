#pragma once
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct pending_bet_service_i : public base_service_i<pending_bet_object>
{
    using pending_bet_call_type = std::function<bool(const base_service_i::object_type&)>;

    virtual void foreach_pending_bets(const game_id_type&, pending_bet_call_type) = 0;

    virtual bool is_exists(const bet_id_type&) const = 0;

    virtual const pending_bet_object& get_by_bet(const bet_id_type&) const = 0;

    virtual const pending_bet_object& get_pending_bet(const pending_bet_id_type&) const = 0;

    virtual view_type get_bets(pending_bet_id_type lower_bound) const = 0;

    virtual std::vector<object_cref_type> get_bets(const game_id_type& game_id) const = 0;
};

class dbs_pending_bet : public dbs_service_base<pending_bet_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_pending_bet(database& db);

public:
    virtual void foreach_pending_bets(const game_id_type&, pending_bet_call_type) override;

    virtual bool is_exists(const bet_id_type&) const override;

    virtual const pending_bet_object& get_by_bet(const bet_id_type&) const override;

    virtual const pending_bet_object& get_pending_bet(const pending_bet_id_type&) const override;

    virtual view_type get_bets(pending_bet_id_type lower_bound) const override;

    virtual std::vector<object_cref_type> get_bets(const game_id_type& game_id) const override;
};
}
}
