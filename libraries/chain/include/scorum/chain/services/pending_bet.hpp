#pragma once
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct pending_bet_service_i : public base_service_i<pending_bet_object>
{
    using pending_bet_call_type = std::function<bool(const base_service_i::object_type&)>;
    using base_service_i<pending_bet_object>::is_exists;

    virtual void foreach_pending_bets(const game_id_type&, pending_bet_call_type) = 0;

    virtual bool is_exists(const pending_bet_id_type& id) const = 0;
    virtual bool is_exists(const uuid_type& uuid) const = 0;
    virtual const pending_bet_object& get_pending_bet(const pending_bet_id_type&) const = 0;
    virtual const pending_bet_object& get_pending_bet(const uuid_type&) const = 0;

    virtual view_type get_bets(pending_bet_id_type lower_bound) const = 0;
    virtual std::vector<object_cref_type> get_bets(game_id_type game_id) const = 0;
    virtual std::vector<object_cref_type> get_bets(game_id_type game_id, pending_bet_kind kind) const = 0;
    virtual std::vector<object_cref_type> get_bets(game_id_type game_id, account_name_type better) const = 0;
    virtual std::vector<object_cref_type> get_bets(game_id_type game_id, fc::time_point_sec created_from) const = 0;
};

class dbs_pending_bet : public dbs_service_base<pending_bet_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_pending_bet(database& db);

public:
    using base_service_i<pending_bet_object>::is_exists;

    virtual void foreach_pending_bets(const game_id_type&, pending_bet_call_type) override;

    bool is_exists(const pending_bet_id_type& id) const override;
    bool is_exists(const uuid_type& uuid) const override;
    const pending_bet_object& get_pending_bet(const pending_bet_id_type&) const override;
    const pending_bet_object& get_pending_bet(const uuid_type&) const override;

    virtual view_type get_bets(pending_bet_id_type lower_bound) const override;
    virtual std::vector<object_cref_type> get_bets(game_id_type game_id) const override;
    virtual std::vector<object_cref_type> get_bets(game_id_type game_id, pending_bet_kind kind) const override;
    std::vector<object_cref_type> get_bets(game_id_type game_id, account_name_type better) const override;
    std::vector<object_cref_type> get_bets(game_id_type game_id, fc::time_point_sec created_from) const override;
};
}
}
