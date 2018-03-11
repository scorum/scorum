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

struct genesis_chain_id_type
{
    chain_id_type initial_chain_id;
};

struct genesis_state_type : public genesis_chain_id_type
{
    struct account_type
    {
        std::string name;
        std::string recovery_account;
        public_key_type public_key;
        asset scr_amount;
    };

    struct founder_type
    {
        std::string name;
        float sp_percent;
    };

    struct steemit_bounty_account_type
    {
        std::string name;
        asset sp_amount;
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
    asset rewards_supply = asset(0, SCORUM_SYMBOL);
    asset founders_supply = asset(0, SP_SYMBOL);
    asset steemit_bounty_accounts_supply = asset(0, SP_SYMBOL);
    asset development_sp_supply = asset(0, SP_SYMBOL);
    asset development_scr_supply = asset(0, SCORUM_SYMBOL);
    time_point_sec initial_timestamp = time_point_sec::min();
    std::vector<account_type> accounts;
    std::vector<founder_type> founders;
    std::vector<steemit_bounty_account_type> steemit_bounty_accounts;
    std::vector<witness_type> witness_candidates;
    std::vector<registration_schedule_item> registration_schedule;
    std::vector<std::string> registration_committee;
};

} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT(scorum::chain::genesis_state_type::account_type,
           (name)
           (recovery_account)
           (public_key)
           (scr_amount))

FC_REFLECT(scorum::chain::genesis_state_type::founder_type,
           (name)
           (sp_percent))

FC_REFLECT(scorum::chain::genesis_state_type::steemit_bounty_account_type,
           (name)
           (sp_amount))

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
           (rewards_supply)
           (founders_supply)
           (steemit_bounty_accounts_supply)
           (development_sp_supply)
           (development_scr_supply)
           (initial_timestamp)
           (accounts)
           (founders)
           (steemit_bounty_accounts)
           (witness_candidates)
           (registration_schedule)
           (registration_committee))
// clang-format on
