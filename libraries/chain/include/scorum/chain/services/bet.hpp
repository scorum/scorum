#pragma once
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct bet_service_i : public base_service_i<bet_object>
{
    virtual const bet_object& get_bet(const bet_id_type&) const = 0;
    virtual view_type get_bets(bet_id_type lower_bound) const = 0;

    virtual bool is_exists(const bet_id_type&) const = 0;

    virtual std::vector<object_cref_type> get_bets(const game_id_type& game_id) const = 0;
};

class dbs_bet : public dbs_service_base<bet_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_bet(database& db);

public:
    virtual const bet_object& get_bet(const bet_id_type&) const override;
    virtual view_type get_bets(bet_id_type lower_bound) const;

    virtual bool is_exists(const bet_id_type&) const override;

    virtual std::vector<object_cref_type> get_bets(const game_id_type& game_id) const override;
};
}
}
