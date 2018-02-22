#pragma once

#include <fc/reflect/reflect.hpp>
#include <fc/time.hpp>

namespace scorum {
namespace common_statistics {

struct by_bucket;

struct base_bucket_object
{
    fc::time_point_sec open; ///< Open time of the bucket
    uint32_t seconds = 0; ///< Seconds accounted for in the bucket
};
}
}

FC_REFLECT(scorum::common_statistics::base_bucket_object, (open)(seconds))
