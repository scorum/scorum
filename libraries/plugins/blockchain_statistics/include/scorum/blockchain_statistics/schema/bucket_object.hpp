#pragma once

#include <boost/multi_index/composite_key.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/blockchain_statistics/schema/metrics.hpp>
#include <scorum/common_statistic/base_bucket_object.hpp>

#ifndef BLOCKCHAIN_STATISTICS_SPACE_ID
#define BLOCKCHAIN_STATISTICS_SPACE_ID 9
#endif

namespace scorum {
namespace blockchain_statistics {

using namespace scorum::chain;

enum blockchain_statistics_object_type
{
    bucket_object_type = (BLOCKCHAIN_STATISTICS_SPACE_ID << 8)
};

struct bucket_object : public common_statistics::base_bucket_object,
                       public base_metric,
                       public object<bucket_object_type, bucket_object>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(bucket_object)

    id_type id;
};

typedef oid<bucket_object> bucket_id_type;

struct by_id;
typedef shared_multi_index_container<bucket_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<bucket_object,
                                                                      bucket_id_type,
                                                                      &bucket_object::id>>,
                                                ordered_unique<tag<common_statistics::by_bucket>,
                                                               composite_key<bucket_object,
                                                                             member<common_statistics::
                                                                                        base_bucket_object,
                                                                                    uint32_t,
                                                                                    &common_statistics::
                                                                                        base_bucket_object::seconds>,
                                                                             member<common_statistics::
                                                                                        base_bucket_object,
                                                                                    fc::time_point_sec,
                                                                                    &common_statistics::
                                                                                        base_bucket_object::open>>>>>
    bucket_index;
} // namespace blockchain_statistics
} // namespace scorum

FC_REFLECT_DERIVED(scorum::blockchain_statistics::bucket_object,
                   (scorum::common_statistics::base_bucket_object)(scorum::blockchain_statistics::base_metric),
                   (id))

CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_statistics::bucket_object, scorum::blockchain_statistics::bucket_index)
