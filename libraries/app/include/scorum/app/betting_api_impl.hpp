#include <scorum/app/betting_api.hpp>

#include <chainbase/database_guard.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/chain/services/betting_property.hpp>

#include <scorum/app/application.hpp>
#include <scorum/common_api/config_api.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm_ext/copy_n.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <scorum/utils/range_adaptors.hpp>

#include <scorum/utils/take_n_range_adaptor.hpp>

namespace scorum {
namespace app {

class betting_api_impl
{
public:
    betting_api_impl(data_service_factory_i& service_factory, uint32_t lookup_limit = LOOKUP_LIMIT)
        : _game_service(service_factory.game_service())
        , _pending_bet_service(service_factory.pending_bet_service())
        , _matched_bet_service(service_factory.matched_bet_service())
        , _betting_property_service(service_factory.betting_property_service())
        , _lookup_limit(lookup_limit)
    {
    }

    std::vector<game_api_object> get_games(game_filter filter) const
    {
        // clang-format off
        auto rng = _game_service.get_games()
                | boost::adaptors::filtered([&](const auto& obj) {
                    return static_cast<uint8_t>(obj.status) & static_cast<uint8_t>(filter); })
                | boost::adaptors::transformed([](const auto& obj) { return game_api_object(obj); });
        // clang-format on

        return { rng.begin(), rng.end() };
    }

    auto get_matched_bets(matched_bet_id_type from, uint32_t limit) const
    {
        return get_bets<matched_bet_api_object>(_matched_bet_service, from, limit);
    }

    auto get_pending_bets(pending_bet_id_type from, uint32_t limit) const
    {
        return get_bets<pending_bet_api_object>(_pending_bet_service, from, limit);
    }

    template <typename ApiObjectType, typename ServiceType, typename IdType>
    std::vector<ApiObjectType> get_bets(ServiceType& service, IdType from, uint32_t limit) const
    {
        FC_ASSERT(limit <= _lookup_limit, "Limit should be le than LOOKUP_LIMIT",
                  ("limit", limit)("LOOKUP_LIMIT", _lookup_limit));

        // clang-format off
        auto rng = service.get_bets(from)
                | utils::adaptors::take_n(limit)
                | boost::adaptors::transformed([](const auto& obj) { return ApiObjectType(obj); });
        // clang-format on

        return { rng.begin(), rng.end() };
    }

    betting_property_api_object get_betting_properties() const
    {
        return _betting_property_service.get();
    }

private:
    chain::game_service_i& _game_service;
    chain::pending_bet_service_i& _pending_bet_service;
    chain::matched_bet_service_i& _matched_bet_service;
    chain::betting_property_service_i& _betting_property_service;

    const uint32_t _lookup_limit;
};

} // namespace app
} // namespace scorum
