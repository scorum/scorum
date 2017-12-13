#pragma once

#include <vector>
#include <string>

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

#include <fc/reflect/reflect.hpp>

namespace scorum {
namespace chain {

namespace sp = scorum::protocol;

struct genesis_state_type
{
    struct account_type
    {
        std::string name;
        std::string recovery_account;
        sp::public_key_type public_key;
        sp::share_type scr_amount;
        sp::share_type sp_amount;
    };

    struct witness_type
    {
        std::string owner_name;
        sp::public_key_type block_signing_key;
    };

    struct registration_schedule_item
    {
        uint8_t stage;
        uint32_t users;
        uint16_t bonus_percent;
    };

    sp::share_type registration_supply = 0;
    sp::share_type registration_maximum_bonus = 0;
    sp::share_type init_supply = 0;
    sp::asset init_rewards_supply;
    time_point_sec initial_timestamp;
    std::vector<account_type> accounts;
    std::vector<witness_type> witness_candidates;
    std::vector<registration_schedule_item> registration_schedule;
    std::vector<std::string> registration_committee;

    sp::chain_id_type initial_chain_id;
};

namespace utils {

void generate_default_genesis_state(genesis_state_type& genesis);

} // namespace utils
} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT(scorum::chain::genesis_state_type::account_type,
           (name)
           (recovery_account)
           (public_key)
           (scr_amount)
           (sp_amount))

FC_REFLECT(scorum::chain::genesis_state_type::witness_type,
           (owner_name)
           (block_signing_key))

FC_REFLECT(scorum::chain::genesis_state_type::registration_schedule_item,
           (stage)
           (users)
           (bonus_percent))

FC_REFLECT(scorum::chain::genesis_state_type,
           (registration_supply)
           (registration_maximum_bonus)
           (init_supply)
           (init_rewards_supply)
           (initial_timestamp)
           (accounts)
           (witness_candidates)
           (registration_schedule)
           (registration_committee)
           (initial_chain_id))
// clang-format on
