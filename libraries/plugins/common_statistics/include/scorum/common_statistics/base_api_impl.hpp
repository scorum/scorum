#pragma once

#include <scorum/common_statistics/base_plugin_impl.hpp>

namespace scorum {
namespace common_statistics {

template <typename Bucket, typename Statistic> class common_statistics_api_impl
{
    typedef typename chainbase::get_index_type<Bucket>::type bucket_index;

public:
    typedef common_statistics_api_impl base_api_impl;

    scorum::app::application& _app;
    const std::string _plugin_name;

public:
    common_statistics_api_impl(scorum::app::application& app, const std::string& name)
        : _app(app)
        , _plugin_name(name)
    {
    }

    Statistic get_stats_for_time(const fc::time_point_sec& open, uint32_t interval) const
    {
        Statistic result;
        const auto& bucket_idx = _app.chain_database()->template get_index<bucket_index, by_bucket>();
        auto itr = bucket_idx.lower_bound(boost::make_tuple(interval, open));

        if (itr != bucket_idx.end())
            result += *itr;

        return result;
    }

    template <typename Plugin>
    Statistic get_stats_for_interval(const fc::time_point_sec& start, const fc::time_point_sec& end) const
    {
        Statistic result;
        const auto& bucket_itr = _app.chain_database()->template get_index<bucket_index, by_bucket>();
        const auto& sizes = _app.get_plugin<Plugin>(_plugin_name)->get_tracked_buckets();
        auto size_itr = sizes.rbegin();
        auto time = start;

        // This is a greedy algorithm, same as the ubiquitous "making change" problem.
        // So long as the bucket sizes share a common denominator, the greedy solution
        // has the same efficiency as the dynamic solution.
        while (size_itr != sizes.rend() && time < end)
        {
            auto itr = bucket_itr.find(boost::make_tuple(*size_itr, time));

            while (itr != bucket_itr.end() && itr->seconds == *size_itr && time + itr->seconds <= end)
            {
                time += *size_itr;
                result += *itr;
                itr++;
            }

            size_itr++;
        }

        return result;
    }

    Statistic get_lifetime_stats() const
    {
        Statistic result;

        const auto& bucket_idx = _app.chain_database()->template get_index<bucket_index, by_bucket>();
        auto itr = bucket_idx.find(boost::make_tuple(LIFE_TIME_PERIOD, fc::time_point_sec()));

        if (itr != bucket_idx.end())
        {
            result += *itr;
        }

        return result;
    }
};

} // namespace common_statistics
} // namespace scorum
