#include <scorum/chain/services/betting_service.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/chain/schema/game_object.hpp>

namespace scorum {
namespace chain {
dbs_betting::dbs_betting(database& db)
    : _base_type(db)
    , _betting_property(db.betting_property_service())
{
}

bool dbs_betting::is_betting_moderator(const account_name_type& account_name) const
{
    try
    {
        return _betting_property.get().moderator == account_name;
    }
    FC_CAPTURE_LOG_AND_RETHROW((account_name))
}

void dbs_betting::return_unresolved_bets(const game_object& game)
{
    boost::ignore_unused_variable_warning(game);
    FC_THROW("not implemented");
}

void dbs_betting::return_bets(const game_object& game, const std::vector<betting::wincase_pair>& cancelled_wincases)
{
    boost::ignore_unused_variable_warning(game);
    boost::ignore_unused_variable_warning(cancelled_wincases);
    FC_THROW("not implemented");
}

void dbs_betting::remove_disputs(const game_object& game)
{
    boost::ignore_unused_variable_warning(game);
    FC_THROW("not implemented");
}

void dbs_betting::remove_bets(const game_object& game)
{
    boost::ignore_unused_variable_warning(game);
    FC_THROW("not implemented");
}
}
}
