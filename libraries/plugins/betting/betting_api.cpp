#include <scorum/betting/betting_api.hpp>
#include <scorum/betting/detail/betting_api_impl.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/app/api_context.hpp>
#include <scorum/app/application.hpp>

namespace scorum {
namespace betting {

betting_api::betting_api(const scorum::app::api_context& ctx)
    : _impl(new detail::betting_api_impl(ctx.app.chain_database()))
{
}

betting_api::~betting_api() = default;

void betting_api::on_api_startup()
{
}

std::vector<api::game_api_obj> betting_api::get_games(int64_t from_id, uint32_t limit) const
{
    try
    {
        return _impl->guard().with_read_lock([&]() { return _impl->get_games(from_id, limit); });
    }
    FC_CAPTURE_AND_RETHROW((from_id)(limit))
}
}
}
