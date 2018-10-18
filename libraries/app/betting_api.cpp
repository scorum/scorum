#include <scorum/app/betting_api.hpp>
#include <scorum/app/betting_api_impl.hpp>

namespace scorum {
namespace app {

betting_api::betting_api(const api_context& ctx)
    : _impl(std::make_unique<betting_api_impl>(*ctx.app.chain_database(),
                                               ctx.app.chain_database()->get_dba<game_object>(),
                                               ctx.app.chain_database()->get_dba<matched_bet_object>()))
    , _guard(ctx.app.chain_database())
{
}

void betting_api::on_api_startup()
{
}

std::vector<winner_api_object> betting_api::get_game_winners(const uuid_type& game_uuid) const
{
    return _guard->with_read_lock([&] { return _impl->get_game_winners(game_uuid); });
}

std::vector<game_api_object> betting_api::get_games(game_filter filter) const
{
    return _guard->with_read_lock([&] { return _impl->get_games(filter); });
}

std::vector<matched_bet_api_object> betting_api::get_matched_bets(matched_bet_id_type from, uint32_t limit) const
{
    return _guard->with_read_lock([&] { return _impl->get_matched_bets(from, limit); });
}

std::vector<pending_bet_api_object> betting_api::get_pending_bets(pending_bet_id_type from, uint32_t limit) const
{
    return _guard->with_read_lock([&] { return _impl->get_pending_bets(from, limit); });
}

betting_property_api_object betting_api::get_betting_properties() const
{
    return _guard->with_read_lock([&] { return _impl->get_betting_properties(); });
}

} // namespace app
} // namespace scorum
