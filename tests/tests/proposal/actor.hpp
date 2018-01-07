#pragma once

#include <scorum/protocol/types.hpp>

namespace sp = scorum::protocol;

using asset = sp::asset;
using private_key_type = sp::private_key_type;
using public_key_type = sp::public_key_type;

class Actor;

using Actors = fc::flat_map<std::string, Actor>;

class Actor
{
public:
    //    Actor() = delete;
    Actor()
    {
    }

    Actor(const char* pszName)
        : name(pszName)
        , private_key(private_key_type::regenerate(fc::sha256::hash(name)))
        , post_key(private_key_type::regenerate(fc::sha256::hash(std::string(name + "_post"))))
        , public_key(private_key.get_public_key())
    {
    }

    Actor(const std::string& name)
        : name(name)
        , private_key(private_key_type::regenerate(fc::sha256::hash(name)))
        , post_key(private_key_type::regenerate(fc::sha256::hash(std::string(name + "_post"))))
        , public_key(private_key.get_public_key())
    {
    }

    Actor(const Actor& a)
    {
        this->name = a.name;
        this->scr_amount = a.scr_amount;
        this->sp_amount = a.sp_amount;
        this->private_key = a.private_key;
        this->public_key = a.public_key;
        this->post_key = a.post_key;
    }

    Actor& scorum(asset scr)
    {
        scr_amount = scr;
        return *this;
    }

    Actor& vests(asset vests)
    {
        sp_amount = vests;
        return *this;
    }

    void vote_for()
    {
    }

    void create_proposal()
    {
    }

    std::string name;

    asset scr_amount;
    asset sp_amount;

    private_key_type private_key;
    private_key_type post_key;
    public_key_type public_key;
};
