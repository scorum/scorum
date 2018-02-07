#pragma once

#include <vector>
#include <string>

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

#include <fc/reflect/reflect.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::chain_id_type;
using scorum::protocol::public_key_type;

struct genesis_state_type
{
    struct account_type
    {
        std::string name;
        std::string recovery_account;
        public_key_type public_key;
        asset scr_amount;
    };

    struct founders_type
    {
        std::string name;
        uint16_t sp_percent;
    };

    struct witness_type
    {
        std::string name;
        public_key_type block_signing_key;
    };

    struct registration_schedule_item
    {
        uint8_t stage;
        uint32_t users;
        uint16_t bonus_percent;
    };

    asset registration_supply = asset(0, SCORUM_SYMBOL);
    asset registration_bonus = asset(0, SCORUM_SYMBOL);
    asset accounts_supply = asset(0, SCORUM_SYMBOL);
    asset init_rewards_supply = asset(0, SCORUM_SYMBOL);
    asset founders_supply = asset(0, VESTS_SYMBOL);
    time_point_sec initial_timestamp;
    std::vector<account_type> accounts;
    std::vector<founders_type> founders;
    std::vector<witness_type> witness_candidates;
    std::vector<registration_schedule_item> registration_schedule;
    std::vector<std::string> registration_committee;

    chain_id_type initial_chain_id;
};

} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT(scorum::chain::genesis_state_type::account_type,
           (name)
           (recovery_account)
           (public_key)
           (scr_amount))

FC_REFLECT(scorum::chain::genesis_state_type::founders_type,
           (name)
           (sp_percent))

FC_REFLECT(scorum::chain::genesis_state_type::witness_type,
          (name)
          (block_signing_key))

FC_REFLECT(scorum::chain::genesis_state_type::registration_schedule_item,
           (stage)
           (users)
           (bonus_percent))

FC_REFLECT(scorum::chain::genesis_state_type,
           (registration_supply)
           (registration_bonus)
           (accounts_supply)
           (init_rewards_supply)
           (founders_supply)
           (initial_timestamp)
           (accounts)
           (founders)
           (witness_candidates)
           (registration_schedule)
           (registration_committee)
           (initial_chain_id))
// clang-format on
