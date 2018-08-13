#pragma once
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/betting/betting_api.hpp>
#include <scorum/betting/betting_api_objects.hpp>

#include <scorum/chain/schema/game_object.hpp>
#include <chainbase/database_guard.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <scorum/utils/take_n_range_adaptor.hpp>

namespace scorum {
namespace betting {
namespace detail {

using namespace scorum::chain;

class betting_api_impl
{
public:
    betting_api_impl(std::shared_ptr<chainbase::database> db)
        : _db(std::move(db))
    {
    }

    std::vector<api::game_api_obj> get_games(int64_t from_id, uint32_t limit) const
    {
        const auto& idx = _db->get_index<game_index, by_id>();

        auto rng = boost::make_iterator_range(idx.lower_bound(from_id), idx.end()) //
            | utils::adaptors::take_n(limit)
            | boost::adaptors::transformed([](const auto& x) { return api::game_api_obj(x); });

        return { rng.begin(), rng.end() };
    }

    chainbase::database_guard& guard() const
    {
        return *_db;
    }

private:
    std::shared_ptr<chainbase::database> _db;
};
}
}
}
