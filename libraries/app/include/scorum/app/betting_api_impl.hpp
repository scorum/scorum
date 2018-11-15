#include <scorum/app/betting_api.hpp>

#include <chainbase/database_guard.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/dba/db_accessor.hpp>

#include <scorum/app/application.hpp>
#include <scorum/common_api/config_api.hpp>

#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <scorum/utils/take_n_range.hpp>
#include <scorum/utils/range/join_range.hpp>
#include <scorum/utils/range/flatten_range.hpp>
#include <scorum/utils/collect_range_adaptor.hpp>
#include <scorum/utils/set_intersection.hpp>

namespace scorum {
namespace app {

using namespace scorum::chain;
using namespace scorum::protocol;

class betting_api::impl
{
public:
    impl(dba::db_accessor<betting_property_object>& betting_prop_dba,
         dba::db_accessor<game_object>& game_dba,
         dba::db_accessor<matched_bet_object>& matched_bet_dba,
         dba::db_accessor<pending_bet_object>& pending_bet_dba,
         uint32_t lookup_limit = LOOKUP_LIMIT)
        : _betting_prop_dba(betting_prop_dba)
        , _game_dba(game_dba)
        , _matched_bet_dba(matched_bet_dba)
        , _pending_bet_dba(pending_bet_dba)
        , _lookup_limit(lookup_limit)
    {
    }
    std::vector<matched_bet_api_object> get_game_returns(const uuid_type& game_uuid) const
    {
        using namespace scorum::chain::dba;

        FC_ASSERT(_game_dba.is_exists_by<by_uuid>(game_uuid), "Game with uuid '${1}' doesn't exist", ("1", game_uuid));
        const auto& game = _game_dba.get_by<by_uuid>(game_uuid);
        auto bets_rng = _matched_bet_dba.get_range_by<by_game_uuid_market>(game_uuid);

        std::vector<matched_bet_api_object> winners;

        for (const matched_bet_object& bet : bets_rng)
        {
            auto fst_won = game.results.find(bet.bet1_data.wincase) != game.results.end();
            auto snd_won = game.results.find(bet.bet2_data.wincase) != game.results.end();

            if (!fst_won && !snd_won)
            {
                winners.push_back(matched_bet_api_object(bet));
            }
        }

        return winners;
    }

    std::vector<winner_api_object> get_game_winners(const uuid_type& game_uuid) const
    {
        using namespace scorum::chain::dba;

        FC_ASSERT(_game_dba.is_exists_by<by_uuid>(game_uuid), "Game with uuid '${1}' doesn't exist", ("1", game_uuid));
        const auto& game = _game_dba.get_by<by_uuid>(game_uuid);
        auto bets_rng = _matched_bet_dba.get_range_by<by_game_uuid_market>(game_uuid);

        std::vector<winner_api_object> winners;

        for (const matched_bet_object& bet : bets_rng)
        {
            auto fst_won = game.results.find(bet.bet1_data.wincase) != game.results.end();
            auto snd_won = game.results.find(bet.bet2_data.wincase) != game.results.end();

            if (fst_won)
            {
                winners.push_back(winner_api_object(bet.market, bet.bet1_data, bet.bet2_data));
            }
            else if (snd_won)
            {
                winners.push_back(winner_api_object(bet.market, bet.bet2_data, bet.bet1_data));
            }
        }

        return winners;
    }

    std::vector<game_api_object> get_games_by_status(const fc::flat_set<game_status>& filter) const
    {
        auto games = _game_dba.get_all_by<by_id>();
        auto rng = games //
            | boost::adaptors::filtered([&](const auto& obj) { return filter.find(obj.status) != filter.end(); })
            | boost::adaptors::transformed([](const auto& obj) { return game_api_object(obj); });

        return { rng.begin(), rng.end() };
    }

    std::vector<game_api_object> get_games_by_uuids(const std::vector<uuid_type>& uuids) const
    {
        using namespace boost::adaptors;
        using namespace utils::adaptors;

        auto result = uuids //
            | filtered([&](auto uid) { return _game_dba.is_exists_by<by_uuid>(uid); })
            | transformed([&](auto uid) { return game_api_object(_game_dba.get_by<by_uuid>(uid)); })
            | collect<std::vector>(uuids.size());

        return result;
    }

    std::vector<game_api_object> lookup_games_by_id(game_id_type from, uint32_t limit) const
    {
        using namespace boost::adaptors;
        using namespace utils::adaptors;

        auto lookup_limit = std::min(limit, _lookup_limit);
        auto games = _game_dba.get_range_by<by_id>(from <= dba::_x, dba::unbounded);

        auto result = games //
            | take_n(lookup_limit) //
            | transformed([&](const auto& obj) { return game_api_object(obj); }) //
            | collect<std::vector>(lookup_limit);

        return result;
    }

    std::vector<matched_bet_api_object> lookup_matched_bets(matched_bet_id_type from, uint32_t limit) const
    {
        return get_bets<matched_bet_api_object>(_matched_bet_dba, from, limit);
    }

    std::vector<pending_bet_api_object> lookup_pending_bets(pending_bet_id_type from, uint32_t limit) const
    {
        return get_bets<pending_bet_api_object>(_pending_bet_dba, from, limit);
    }

    std::vector<matched_bet_api_object> get_matched_bets(const std::vector<uuid_type>& uuids) const
    {
        using namespace boost::adaptors;
        using namespace utils::adaptors;

        auto with_bet1_matched = uuids
            | transformed([&](auto uid) { return _matched_bet_dba.get_range_by<by_bet1_uuid>(uid); }) //
            | flatten;

        auto with_bet2_matched = uuids
            | transformed([&](auto uid) { return _matched_bet_dba.get_range_by<by_bet2_uuid>(uid); }) //
            | flatten;

        auto matched_bets_vec = with_bet1_matched //
            | joined(with_bet2_matched) //
            | transformed([](const auto& bet) { return matched_bet_api_object(bet); }) //
            | collect<std::vector>();

        boost::range::sort(matched_bets_vec, [](const auto& l, const auto& r) { return l.id < r.id; });
        auto rng = boost::range::unique(matched_bets_vec, [](const auto& l, const auto& r) { return l.id == r.id; });

        matched_bets_vec.erase(rng.end(), matched_bets_vec.end());

        return matched_bets_vec;
    }

    std::vector<pending_bet_api_object> get_pending_bets(const std::vector<uuid_type>& uuids) const
    {
        using namespace boost::adaptors;
        using namespace utils::adaptors;

        auto result = uuids //
            | filtered([&](auto uid) { return _pending_bet_dba.is_exists_by<by_uuid>(uid); })
            | transformed([&](auto uid) { return pending_bet_api_object(_pending_bet_dba.get_by<by_uuid>(uid)); })
            | collect<std::vector>(uuids.size());

        return result;
    }

    template <typename TApiObject, typename TObject, typename TId>
    std::vector<TApiObject> get_bets(dba::db_accessor<TObject>& accessor, TId from, uint32_t limit) const
    {
        using namespace dba;
        using namespace boost::adaptors;
        using namespace utils::adaptors;

        FC_ASSERT(limit <= _lookup_limit, "Limit should be le than LOOKUP_LIMIT",
                  ("limit", limit)("LOOKUP_LIMIT", _lookup_limit));

        auto bets = accessor.template get_range_by<by_id>(from <= _x, unbounded);
        auto result = bets //
            | take_n(limit) //
            | transformed([](const auto& obj) { return TApiObject(obj); }) //
            | collect<std::vector>();

        return result;
    }

    betting_property_api_object get_betting_properties() const
    {
        return _betting_prop_dba.get();
    }

private:
    dba::db_accessor<betting_property_object>& _betting_prop_dba;
    dba::db_accessor<game_object>& _game_dba;
    dba::db_accessor<matched_bet_object>& _matched_bet_dba;
    dba::db_accessor<pending_bet_object>& _pending_bet_dba;

    const uint32_t _lookup_limit;
};

} // namespace app
} // namespace scorum
