#include <scorum/app/betting_api.hpp>
#include <scorum/app/betting_api_impl.hpp>

namespace scorum {
namespace app {

using namespace scorum::chain;
using namespace scorum::protocol;

betting_api::betting_api(const api_context& ctx)
    : _impl(std::make_unique<impl>(ctx.app.chain_database()->get_dba<betting_property_object>(),
                                   ctx.app.chain_database()->get_dba<game_object>(),
                                   ctx.app.chain_database()->get_dba<matched_bet_object>(),
                                   ctx.app.chain_database()->get_dba<pending_bet_object>()))
    , _guard(ctx.app.chain_database())
{
}

betting_api::~betting_api() = default;

void betting_api::on_api_startup()
{
}

std::vector<matched_bet_api_object> betting_api::get_game_returns(const uuid_type& game_uuid) const
{
    return _guard->with_read_lock([&] { return _impl->get_game_returns(game_uuid); });
}

std::vector<winner_api_object> betting_api::get_game_winners(const uuid_type& game_uuid) const
{
    return _guard->with_read_lock([&] { return _impl->get_game_winners(game_uuid); });
}

std::vector<game_api_object> betting_api::get_games_by_status(const fc::flat_set<chain::game_status>& filter) const
{
    return _guard->with_read_lock([&] { return _impl->get_games_by_status(filter); });
}

std::vector<game_api_object> betting_api::get_games_by_uuids(const std::vector<uuid_type>& uuids) const
{
    return _guard->with_read_lock([&] { return _impl->get_games_by_uuids(uuids); });
}

std::vector<game_api_object> betting_api::lookup_games_by_id(game_id_type from, uint32_t limit) const
{
    return _guard->with_read_lock([&] { return _impl->lookup_games_by_id(from, limit); });
}

std::vector<matched_bet_api_object> betting_api::lookup_matched_bets(matched_bet_id_type from, uint32_t limit) const
{
    return _guard->with_read_lock([&] { return _impl->lookup_matched_bets(from, limit); });
}

std::vector<pending_bet_api_object> betting_api::lookup_pending_bets(pending_bet_id_type from, uint32_t limit) const
{
    return _guard->with_read_lock([&] { return _impl->lookup_pending_bets(from, limit); });
}

std::vector<matched_bet_api_object> betting_api::get_matched_bets(const std::vector<uuid_type>& uuids) const
{
    return _guard->with_read_lock([&] { return _impl->get_matched_bets(uuids); });
}

std::vector<pending_bet_api_object> betting_api::get_pending_bets(const std::vector<uuid_type>& uuids) const
{
    return _guard->with_read_lock([&] { return _impl->get_pending_bets(uuids); });
}

betting_property_api_object betting_api::get_betting_properties() const
{
    return _guard->with_read_lock([&] { return _impl->get_betting_properties(); });
}

} // namespace app
} // namespace scorum
