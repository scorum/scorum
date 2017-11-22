#pragma once

#include <vector>
#include <string>

#include <scorum/protocol/types.hpp>

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
        uint64_t scr_amount;
        uint64_t sp_amount;
    };

    struct witness_type
    {
        std::string owner_name;
        sp::public_key_type block_signing_key;
    };

    genesis_state_type()
        : init_supply(0)
    {
    }

    genesis_state_type(uint64_t supply)
        : init_supply(supply)
    {
    }

    uint64_t init_supply;
    time_point_sec initial_timestamp;
    std::vector<account_type> accounts;
    std::vector<witness_type> witness_candidates;

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

FC_REFLECT(scorum::chain::genesis_state_type,
           (init_supply)
           (initial_timestamp)
           (accounts)
           (witness_candidates)
           (initial_chain_id))
// clang-format on
