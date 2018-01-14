#pragma once

#include <fc/fixed_string.hpp>

#include <scorum/protocol/asset.hpp>

#include <scorum/chain/scorum_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <scorum/protocol/types.hpp>

#include <fc/shared_buffer.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class registration_pool_object : public object<registration_pool_object_type, registration_pool_object>
{
public:
    template <typename Constructor, typename Allocator>
    registration_pool_object(Constructor&& c, allocator<Allocator> a)
        : schedule_items(a.get_segment_manager())
    {
        c(*this);
    }

    id_type id;

    asset balance = asset(0, SCORUM_SYMBOL);

    asset maximum_bonus = asset(0, SCORUM_SYMBOL);

    uint64_t already_allocated_count = 0;

    struct schedule_item
    {
        uint32_t users;

        uint16_t bonus_percent;
    };

    fc::shared_vector<schedule_item> schedule_items;

    uint64_t invite_quorum = SCORUM_COMMITTEE_QUORUM_PERCENT;
    uint64_t dropout_quorum = SCORUM_COMMITTEE_QUORUM_PERCENT;
    uint64_t change_quorum = SCORUM_COMMITTEE_QUORUM_PERCENT;
};

class registration_committee_member_object
    : public object<registration_committee_member_object_type, registration_committee_member_object>
{
    registration_committee_member_object() = delete;

public:
    typedef std::reference_wrapper<const registration_committee_member_object> cref_type;

    template <typename Constructor, typename Allocator>
    registration_committee_member_object(Constructor&& c, allocator<Allocator>)
    {
        c(*this);
    }

    id_type id;

    account_name_type account;

    // temporary schedule info

    asset already_allocated_cash = asset(0, SCORUM_SYMBOL);

    uint32_t last_allocated_block = 0;

    uint32_t per_n_block_remain = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
};

typedef multi_index_container<registration_pool_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<registration_pool_object,
                                                               registration_pool_id_type,
                                                               &registration_pool_object::id>>>,
                              allocator<registration_pool_object>>
    registration_pool_index;

struct by_account_name;

typedef multi_index_container<registration_committee_member_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<registration_committee_member_object,
                                                               registration_committee_member_id_type,
                                                               &registration_committee_member_object::id>>,
                                         ordered_unique<tag<by_account_name>,
                                                        member<registration_committee_member_object,
                                                               account_name_type,
                                                               &registration_committee_member_object::account>>>,
                              allocator<registration_committee_member_object>>
    registration_committee_member_index;
} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::registration_pool_object,
           (id)(balance)(maximum_bonus)(already_allocated_count)(schedule_items))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::registration_pool_object, scorum::chain::registration_pool_index)

FC_REFLECT(scorum::chain::registration_committee_member_object,
           (id)(account)(already_allocated_cash)(last_allocated_block)(per_n_block_remain))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::registration_committee_member_object,
                         scorum::chain::registration_committee_member_index)
