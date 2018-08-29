#include <scorum/app/betting_api.hpp>

#include <chainbase/database_guard.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/app/application.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm_ext/copy_n.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <scorum/utils/range_adaptors.hpp>

#include <scorum/utils/take_n_range_adaptor.hpp>

namespace scorum {
namespace app {

#define BETTING_API_MAX_QUERY_LIMIT 1000

class betting_api_impl
{
public:
    betting_api_impl(data_service_factory_i& service_factory, size_t max_query_limit = BETTING_API_MAX_QUERY_LIMIT)
        : _game_service(service_factory.game_service())
        , _bet_service(service_factory.bet_service())
        , _pending_bet_service(service_factory.pending_bet_service())
        , _matched_bet_service(service_factory.matched_bet_service())
        , _max_query_limit(max_query_limit)
    {
    }

    std::vector<game_api_object> get_games(game_filter filter) const
    {
        // clang-format off
        auto rng = _game_service.get_games()
                | boost::adaptors::filtered([&](const auto& obj) {
                    if (static_cast<uint8_t>(obj.status) & static_cast<uint8_t>(filter))
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                })
                | boost::adaptors::transformed([](const auto& obj) { return game_api_object(obj); });
        // clang-format on

        return { rng.begin(), rng.end() };
    }

    auto get_user_bets(bet_id_type from, int64_t limit) const
    {
        return get_bets<bet_api_object>(_bet_service, from, limit);
    }

    auto get_matched_bets(matched_bet_id_type from, int64_t limit) const
    {
        return get_bets<matched_bet_api_object>(_matched_bet_service, from, limit);
    }

    auto get_pending_bets(pending_bet_id_type from, int64_t limit) const
    {
        return get_bets<pending_bet_api_object>(_pending_bet_service, from, limit);
    }

    template <typename ApiObjectType, typename ServiceType, typename IdType>
    std::vector<ApiObjectType> get_bets(ServiceType& service, IdType from, int64_t limit) const
    {
        FC_ASSERT(limit >= 0, "Limit can't be negative", ("limit", limit));

        const size_t query_limit = static_cast<size_t>(limit);

        FC_ASSERT(query_limit <= _max_query_limit, "Limit should be le than MAX_QUERY_LIMIT",
                  ("limit", limit)("BETTING_API_MAX_QUERY_LIMIT", _max_query_limit));

        // clang-format off
        auto rng = service.get_bets(from)
                | utils::adaptors::take_n(query_limit)
                | boost::adaptors::transformed([](const auto& obj) { return ApiObjectType(obj); });
        // clang-format on

        return { rng.begin(), rng.end() };
    }

private:
    chain::game_service_i& _game_service;
    chain::bet_service_i& _bet_service;
    chain::pending_bet_service_i& _pending_bet_service;
    chain::matched_bet_service_i& _matched_bet_service;

    const size_t _max_query_limit = 1000;
};

} // namespace app
} // namespace scorum
