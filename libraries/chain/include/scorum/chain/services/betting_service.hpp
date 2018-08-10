#pragma once

#include <scorum/chain/services/dbs_base.hpp>

// This is the new none dbs-service
// Look BLOC-415 and refactor this class
// ===========================================
// It must not inherit dbs_base,
// replace dbs_betting to betting_service_impl

namespace scorum {
namespace chain {

struct betting_service_i
{
    virtual bool is_betting_moderator(const account_name_type& account_name) const = 0;
    virtual void return_unresolved_bets(const game_object& game) = 0;
    virtual void return_bets(const game_object& game, const std::vector<betting::wincase_pair>& cancelled_wincases) = 0;
    virtual void remove_disputs(const game_object& game) = 0;
    virtual void remove_bets(const game_object& game) = 0;
};

namespace dba {
template <typename> struct db_accessor_i;
}

class dbs_betting : public dbs_base, public betting_service_i
{
    friend class dbservice_dbs_factory;

protected:
    dbs_betting(database& db);

public:
    virtual bool is_betting_moderator(const account_name_type& account_name) const override;
    virtual void return_unresolved_bets(const game_object& game) override;
    virtual void return_bets(const game_object& game,
                             const std::vector<betting::wincase_pair>& cancelled_wincases) override;
    virtual void remove_disputs(const game_object& game) override;
    virtual void remove_bets(const game_object& game) override;

private:
    dba::db_accessor_i<betting_property_object>& _betting_property;
};
}
}
