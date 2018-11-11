#pragma once

#include "defines.hpp"

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

namespace sp = scorum::protocol;

using asset = sp::asset;
using private_key_type = sp::private_key_type;
using public_key_type = sp::public_key_type;

class Actor;

using Actors = fc::flat_map<std::string, Actor>;

class Actor
{
public:
    Actor();

    Actor(const char* pszName);

    Actor(const std::string& name);

    Actor(const Actor& a);

    Actor& scorum(asset scr);

    Actor& scorumpower(asset scorumpower);

    Actor& percent(float prc);

    operator const std::string&() const;

    std::string name;
    asset scr_amount = asset(0, SCORUM_SYMBOL);
    asset sp_amount = asset(0, SP_SYMBOL);
    float sp_percent = 0.f;
    private_key_type private_key;
    private_key_type post_key;
    public_key_type public_key;
};

class supply_type
{
public:
    using value_type = scorum::protocol::share_value_type;

    supply_type(value_type amount)
        : _amount(amount)
    {
    }

    value_type take(value_type amount)
    {
        amount *= 1'000'000'000;

        FC_ASSERT(_amount >= amount);

        _amount -= amount;

        return amount;
    }

    value_type take_rest()
    {
        value_type a = _amount;

        _amount = 0;

        return a;
    }

private:
    value_type _amount;
};
