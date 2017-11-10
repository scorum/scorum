#pragma once

#include <vector>
#include <string>

#include <scorum/protocol/types.hpp>

#include <fc/reflect/reflect.hpp>

namespace scorum {
namespace chain {

struct genesis_state_type
{
    struct account_type
    {
        std::string name;
        scorum::protocol::public_key_type public_key;
        uint64_t scr_amount;
        uint64_t sp_amount;
    };

    struct witness_type
    {
        std::string name;
        uint64_t votes;
        uint64_t balance;
        scorum::protocol::public_key_type public_key;
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
    std::vector<account_type> accounts;
    std::vector<witness_type> witness_candidates;
};
}
} // namespace scorum::chain

FC_REFLECT(scorum::chain::genesis_state_type::account_type, (name)(public_key)(scr_amount)(sp_amount))
FC_REFLECT(scorum::chain::genesis_state_type::witness_type, (name)(votes)(balance)(public_key))

FC_REFLECT(scorum::chain::genesis_state_type, (init_supply)(accounts)(witness_candidates))
