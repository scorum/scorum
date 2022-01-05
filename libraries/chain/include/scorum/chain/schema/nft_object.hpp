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
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(nft_object, (json_metadata));

    id_type id;
    uuid_type uuid;
    account_name_type owner;
    account_name_type name;
    share_type power;

    fc::shared_string json_metadata;
    time_point_sec created;
};

class game_round_object : public object<game_round_object_type, game_round_object>
{
public:
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(game_round_object, (seed)(verification_key)(vrf)(proof));

    id_type id;
    uuid_type uuid;
    account_name_type owner;

    fc::shared_string seed;
    fc::shared_string verification_key;
    fc::shared_string vrf;
    fc::shared_string proof;
    share_type result;
};

struct by_uuid;

// clang-format off
using nft_index = shared_multi_index_container<
        nft_object, indexed_by<
            ordered_unique<tag<by_id>, member<nft_object, nft_id_type, &nft_object::id>>,
            hashed_unique<tag<by_uuid>, member<nft_object, uuid_type, &nft_object::uuid>>,
            hashed_unique<tag<by_name>, member<nft_object, account_name_type, &nft_object::name>>
        >>;

using game_round_index = shared_multi_index_container<
    game_round_object, indexed_by<
        ordered_unique<tag<by_id>, member<game_round_object, game_round_id_type, &game_round_object::id>>,
        hashed_unique<tag<by_uuid>, member<game_round_object, uuid_type, &game_round_object::uuid>>
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

FC_REFLECT(scorum::chain::game_round_object,
    (id)
    (uuid)
    (owner)
    (seed)
    (verification_key)
    (vrf)
    (proof)
    (result))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::game_round_object, scorum::chain::game_round_index)