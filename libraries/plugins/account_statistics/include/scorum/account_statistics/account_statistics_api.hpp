#pragma once

#include <scorum/account_statistics/schema/metrics.hpp>

#include <fc/api.hpp>

#ifndef API_ACCOUNT_STATISTICS
#define API_ACCOUNT_STATISTICS "account_statistics_api"
#endif

namespace scorum {
namespace app {
struct api_context;
}
} // namespace scorum

namespace scorum {
namespace account_statistics {

namespace detail {
class account_statistics_api_impl;
}

/**
 * @brief Provide api to get statistics over the time window for a particular account
 *
 * Require: account_statistics_plugin
 *
 * @ingroup api
 * @ingroup account_statistics_plugin
 * @addtogroup account_statistics_api Account statistics API
 */
class account_statistics_api
{
public:
    account_statistics_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    /// @name Public API
    /// @addtogroup account_statistics_api
    /// @{

    /**
     * @brief Gets statistics over the time window length, interval, that contains time, open.
     * @param open The opening time, or a time contained within the window.
     * @param interval The size of the window for which statistics were aggregated.
     * @returns Statistics for the window.
     */
    statistics get_stats_for_time(const fc::time_point_sec& open, uint32_t interval) const;

    /**
     * @brief Aggregates statistics over a time interval.
     * @param start The beginning time of the window.
     * @param stop The end time of the window. stop must take place after start.
     * @returns Aggregated statistics over the interval.
     */
    statistics get_stats_for_interval(const fc::time_point_sec& start, const fc::time_point_sec& end) const;

    /**
     * @brief Returns lifetime statistics.
     */
    statistics get_lifetime_stats() const;

    statistics get_stats_for_time_by_account_name(const account_name_type& account_name,
                                                  const fc::time_point_sec& open,
                                                  uint32_t interval) const;
    statistics get_stats_for_interval_by_account_name(const account_name_type& account_name,
                                                      const fc::time_point_sec& start,
                                                      const fc::time_point_sec& end) const;
    statistics get_lifetime_stats_by_account_name(const account_name_type& account_name) const;

    /// @}

private:
    std::shared_ptr<detail::account_statistics_api_impl> my;
};
} // namespace account_statistics
} // namespace scorum

FC_API(scorum::account_statistics::account_statistics_api,
       (get_stats_for_time)(get_stats_for_interval)(get_lifetime_stats)(get_stats_for_time_by_account_name)(
           get_stats_for_interval_by_account_name)(get_lifetime_stats_by_account_name))
