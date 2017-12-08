#pragma once

#include <fc/fixed_string.hpp>

#include <scorum/protocol/asset.hpp>

#include <scorum/chain/scorum_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <scorum/protocol/types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class registration_pool_object : public object<registration_pool_object_type, registration_pool_object>
{
    registration_pool_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    registration_pool_object(Constructor&& c, allocator<Allocator> a)
        : schedule_items(a)
    {
        c(*this);
    }

    struct schedule_item
    {
        uint16_t users_thousands;
        uint16_t bonus_percent;
    };

    id_type id;

    asset balance = asset(0, VESTS_SYMBOL);

    uint64_t already_allocated_count = 0;

    // schedule
    share_type maximum_bonus = 0;
    bip::vector<registration_pool_object::schedule_item, allocator<registration_pool_object::schedule_item>>
        schedule_items;
};

class registration_committee_object : public object<registration_committee_object_type, registration_committee_object>
{
    registration_committee_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    registration_committee_object(Constructor&& c, allocator<Allocator> a)
        : members(a)
    {
        c(*this);
    }

    id_type id;

    bip::set<account_name_type, std::less<account_name_type>, allocator<account_name_type>> members;
};

class registration_committee_member_object
    : public object<registration_committee_member_object_type, registration_committee_member_object>
{
    registration_committee_member_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    registration_committee_member_object(Constructor&& c, allocator<Allocator>)
    {
        c(*this);
    }

    id_type id;

    account_name_type account;

    // cash info
    share_type already_allocated_cash = 0;
    uint32_t last_allocated_block = 0;
};

typedef multi_index_container<registration_pool_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<registration_pool_object,
                                                               registration_pool_id_type,
                                                               &registration_pool_object::id>>>,
                              allocator<registration_pool_object>>
    registration_pool_index;

typedef multi_index_container<registration_committee_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<registration_committee_object,
                                                               registration_committee_id_type,
                                                               &registration_committee_object::id>>>,
                              allocator<registration_committee_object>>
    registration_committee_index;

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
}
}

FC_REFLECT(scorum::chain::registration_pool_object,
           (id)(balance)(already_allocated_count)(maximum_bonus)(schedule_items))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::registration_pool_object, scorum::chain::registration_pool_index)

FC_REFLECT(scorum::chain::registration_committee_object, (id)(members))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::registration_committee_object, scorum::chain::registration_committee_index)

FC_REFLECT(scorum::chain::registration_committee_member_object,
           (id)(account)(already_allocated_cash)(last_allocated_block))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::registration_committee_member_object,
                         scorum::chain::registration_committee_member_index)
