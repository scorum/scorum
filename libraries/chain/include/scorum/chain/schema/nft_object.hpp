#pragma once

#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>
#include <fc/uint128.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/protocol/types.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

namespace scorum {
namespace chain {

class nft_object : public object<nft_object_type, nft_object>
{
public:
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(nft_object, (json_metadata))

    id_type id;
    uuid_type uuid;
    account_name_type owner;
    account_name_type name;
    share_type power;

    fc::shared_string json_metadata;
    time_point_sec created;
};

struct by_uuid;

// clang-format off
using nft_index = shared_multi_index_container<
        nft_object, indexed_by<
            ordered_unique<tag<by_id>, member<nft_object, nft_id_type, &nft_object::id>>,
            hashed_unique<tag<by_uuid>, member<nft_object, uuid_type, &nft_object::uuid>>,
            hashed_unique<tag<by_name>, member<nft_object, account_name_type, &nft_object::name>>
        >>;
// clang-format on

} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::nft_object,
    (id)
    (uuid)
    (owner)
    (name)
    (power)
    (json_metadata)
    (created))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::nft_object, scorum::chain::nft_index)
