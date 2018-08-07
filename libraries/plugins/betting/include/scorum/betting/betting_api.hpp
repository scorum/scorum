#pragma once
#include <fc/api.hpp>
#include <scorum/betting/betting_api_objects.hpp>

#ifndef API_BETTING
#define API_BETTING "betting"
#endif

namespace scorum {
namespace app {
struct api_context;
}
namespace betting {
namespace detail {
class betting_api_impl;
}

/**
 * @brief Provide api for getting info about betting games & bets
 *
 * Require: betting_plugin
 *
 * @ingroup api
 * @ingroup betting_plugin
 * @addtogroup betting_api Betting API
 */
class betting_api
{
public:
    betting_api(const scorum::app::api_context& ctx);
    ~betting_api();

    void on_api_startup();

    /// @name Public API
    /// @addtogroup betting_api
    /// @{

    /**
     * @brief Return non-cancelled games with pagination
     * @param from_id used for pagination
     * @param limit query limit
     * @return array of game_api_obj
     */
    std::vector<api::game_api_obj> get_games(int64_t from_id, uint32_t limit) const;

    /// @}
private:
    std::unique_ptr<detail::betting_api_impl> _impl;
};
}
}

FC_API(scorum::betting::betting_api, (get_games))
