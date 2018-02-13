#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>

#include "actor.hpp"

namespace sp = scorum::protocol;
namespace sc = scorum::chain;

using asset = sp::asset;
using private_key_type = sp::private_key_type;
using public_key_type = sp::public_key_type;
using genesis_state_type = sc::genesis_state_type;

using registration_stage = genesis_state_type::registration_schedule_item;

class Genesis
{
public:
    void account_create(Actor& a)
    {
        if (_accounts.find(a.name) == _accounts.end())
        {
            _accounts.insert(std::make_pair(a.name, a));
            genesis_state.accounts.push_back({ a.name, "", a.public_key, a.scr_amount });
        }
    }

    void founder_create(Actor& a)
    {
        genesis_state.founders.push_back({ a.name, a.sp_percent });
    }

    void steemit_bounty_account_create(Actor& a)
    {
        genesis_state.steemit_bounty_accounts.push_back({ a.name, a.sp_amount });
    }

    void witness_create(Actor& a)
    {
        genesis_state.witness_candidates.push_back({ a.name, a.public_key });
    }

    void committee_member_create(Actor& a)
    {
        genesis_state.registration_committee.push_back(a.name);
    }

public:
    static Genesis create(const genesis_state_type& gs = genesis_state_type())
    {
        Genesis g;
        g.genesis_state = gs;
        return g;
    }

    Genesis()
    {
    }

    template <typename... Args> Genesis& accounts(Args... args)
    {
        std::array<Actor, sizeof...(args)> list = { args... };
        for (Actor& a : list)
        {
            account_create(a);
        }
        return *this;
    }

    template <typename... Args> Genesis& founders(Args... args)
    {
        std::array<Actor, sizeof...(args)> list = { args... };
        for (Actor& a : list)
        {
            account_create(a);
            founder_create(a);
        }
        return *this;
    }

    template <typename... Args> Genesis& steemit_bounty_accounts(Args... args)
    {
        std::array<Actor, sizeof...(args)> list = { args... };
        for (Actor& a : list)
        {
            account_create(a);
            steemit_bounty_account_create(a);
        }
        return *this;
    }

    template <typename... Args> Genesis& committee(Args... args)
    {
        std::array<Actor, sizeof...(args)> list = { args... };
        for (Actor& a : list)
        {
            account_create(a);
            committee_member_create(a);
        }
        return *this;
    }

    template <typename... Args> Genesis& witnesses(Args... args)
    {
        std::array<Actor, sizeof...(args)> list = { args... };
        for (Actor& a : list)
        {
            account_create(a);
            witness_create(a);
        }
        return *this;
    }

    Genesis& registration_supply(asset amount)
    {
        genesis_state.registration_supply = amount;
        return *this;
    }

    Genesis& registration_bonus(asset amount)
    {
        genesis_state.registration_bonus = amount;
        return *this;
    }

    Genesis& accounts_supply(asset amount)
    {
        genesis_state.accounts_supply = amount;
        return *this;
    }

    Genesis& rewards_supply(asset amount)
    {
        genesis_state.rewards_supply = amount;
        return *this;
    }

    Genesis& founders_supply(asset amount)
    {
        genesis_state.founders_supply = amount;
        return *this;
    }

    Genesis& steemit_bounty_accounts_supply(asset amount)
    {
        genesis_state.steemit_bounty_accounts_supply = amount;
        return *this;
    }

    template <typename... Args> Genesis& registration_schedule(Args... args)
    {
        std::array<registration_stage, sizeof...(args)> list = { args... };
        for (registration_stage& s : list)
        {
            genesis_state.registration_schedule.push_back(s);
        }
        return *this;
    }

    genesis_state_type generate();

    Actors _accounts;

private:
    genesis_state_type genesis_state;
};
