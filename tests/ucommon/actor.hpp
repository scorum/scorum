#pragma once

#include "defines.hpp"

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/proposal_object.hpp> //(-)

namespace sp = scorum::protocol;

using asset = sp::asset;
using private_key_type = sp::private_key_type;
using public_key_type = sp::public_key_type;

using account_object = scorum::chain::proposal_object; //(-)

namespace utests {
class account_object_container
{
protected:
    account_object_container()
        : INIT_MEMBER_OBJ(_obj)
    {
    }

public:
    account_object& obj()
    {
        return _obj;
    }

private:
    account_object _obj;
};
}

class Actor;

using Actors = fc::flat_map<std::string, Actor>;

class Actor : utests::account_object_container
{
public:
    Actor();

    Actor(const char* pszName);

    Actor(const std::string& name);

    Actor(const Actor& a);

    Actor& scorum(asset scr);

    Actor& vests(asset vests);

    std::string name;
    asset scr_amount;
    asset sp_amount;
    uint16_t sp_percent;
    private_key_type private_key;
    private_key_type post_key;
    public_key_type public_key;
};
