#pragma once

#include <scorum/blockchain_statistics/api/statistic_api_object.hpp>
#include <fc/api.hpp>

namespace scorum {
namespace app {
struct api_context;
}
}

namespace scorum {
namespace blockchain_statistics {

namespace detail {
class blockchain_statistics_api_impl;
}

class blockchain_statistics_api
{
public:
    blockchain_statistics_api(const scorum::app::api_context& ctx);

    void on_api_startup();

    /**
    * @brief Gets statistics over the time window length, interval, that contains time, open.
    * @param open The opening time, or a time contained within the window.
    * @param interval The size of the window for which statistics were aggregated.
    * @returns Statistics for the window.
    */
    statistics get_stats_for_time(fc::time_point_sec open, uint32_t interval) const;

    /**
    * @brief Aggregates statistics over a time interval.
    * @param start The beginning time of the window.
    * @param stop The end time of the window. stop must take place after start.
    * @returns Aggregated statistics over the interval.
    */
    statistics get_stats_for_interval(fc::time_point_sec start, fc::time_point_sec end) const;

    /**
     * @brief Returns lifetime statistics.
     */
    statistics get_lifetime_stats() const;

private:
    std::shared_ptr<detail::blockchain_statistics_api_impl> my;
};
}
} // scorum::blockchain_statistics

FC_API(scorum::blockchain_statistics::blockchain_statistics_api,
       (get_stats_for_time)(get_stats_for_interval)(get_lifetime_stats))
