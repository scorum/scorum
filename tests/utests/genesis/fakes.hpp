#pragma once

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include "actor.hpp"

using namespace scorum::chain;
using namespace scorum::chain::genesis;
using namespace scorum::protocol;

struct fake_account_object
{
    fake_account_object()
        : config()
    {
    }

    fake_account_object(const char* pszName)
        : config(pszName)
    {
    }

    fake_account_object(const std::string& name)
        : config(name)
    {
    }

    fake_account_object(const Actor& a)
        : config(a)
    {
    }

    account_name_type name;
    asset balance = asset(0, SCORUM_SYMBOL);
    asset vesting_shares = asset(0, VESTS_SYMBOL);

    Actor config;
};
