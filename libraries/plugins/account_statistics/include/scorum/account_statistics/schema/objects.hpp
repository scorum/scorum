#include <boost/multi_index/composite_key.hpp>

#include <scorum/account_statistics/schema/metrics.hpp>

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef ACCOUNT_STATISTICS_SPACE_ID
#define ACCOUNT_STATISTICS_SPACE_ID 10
#endif

namespace scorum {
namespace account_statistics {

using namespace scorum::chain;

enum account_statistics_plugin_object_types
{
    account_stats_bucket_object_type = (ACCOUNT_STATISTICS_SPACE_ID << 8),
    account_activity_bucket_object_type
};

struct account_stats_bucket_object : public account_metric,
                                     public object<account_stats_bucket_object_type, account_stats_bucket_object>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(account_stats_bucket_object)

    id_type id;

    fc::time_point_sec open; ///< Open time of the bucket
    uint32_t seconds = 0; ///< Seconds accounted for in the bucket
};

struct account_activity_bucket_object
    : public object<account_activity_bucket_object_type, account_activity_bucket_object>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(account_activity_bucket_object)

    id_type id;

    fc::time_point_sec open; ///< Open time for the bucket
    uint32_t seconds = 0; ///< Seconds accounted for in the bucket
    uint32_t active_market_accounts = 0; ///< Active market accounts in the bucket
    uint32_t active_forum_accounts = 0; ///< Active forum accounts in the bucket
    uint32_t active_market_and_forum_accounts = 0; ///< Active accounts in both the market and the forum
};
}
} // scorum::account_statistics

FC_REFLECT_DERIVED(scorum::account_statistics::account_stats_bucket_object,
                   (scorum::account_statistics::account_metric),
                   (id)(open)(seconds))

// clang-format off

FC_REFLECT(
    scorum::account_statistics::account_activity_bucket_object,
    (id)
    (open)
    (seconds)
    (active_market_accounts)
    (active_forum_accounts)
    (active_market_and_forum_accounts)
)

// clang-format on
