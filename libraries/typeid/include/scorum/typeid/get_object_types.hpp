#pragma once

#include <stdint.h>

namespace scorum {

/** this class is meant to be specified to enable lookup of index type by object type using
* the SET_INDEX_TYPE macro.
**/
template <typename T> struct get_index_type
{
};

struct empty_object_type
{
};

template <uint16_t Id> struct get_object_type
{
    using type = empty_object_type;
};

struct by_id;
}

#define OBJECT_TYPE_SPACE_ID_OFFSET 6

/**
*  This macro must be used at global scope and OBJECT_TYPE and INDEX_TYPE must be fully qualified
*/
#define CHAINBASE_SET_INDEX_TYPE(OBJECT_TYPE, INDEX_TYPE)                                                              \
    namespace scorum {                                                                                                 \
    template <> struct get_index_type<OBJECT_TYPE>                                                                     \
    {                                                                                                                  \
        using type = INDEX_TYPE;                                                                                       \
    };                                                                                                                 \
    template <> struct get_object_type<OBJECT_TYPE::type_id>                                                           \
    {                                                                                                                  \
        using type = OBJECT_TYPE;                                                                                      \
    };                                                                                                                 \
    }
