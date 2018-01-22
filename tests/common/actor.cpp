#include "actor.hpp"

Actor::Actor()
{
}

Actor::Actor(const char* pszName)
    : name(pszName)
    , private_key(private_key_type::regenerate(fc::sha256::hash(name)))
    , post_key(private_key_type::regenerate(fc::sha256::hash(std::string(name + "_post"))))
    , public_key(private_key.get_public_key())
{
}

Actor::Actor(const std::string& name)
    : name(name)
    , private_key(private_key_type::regenerate(fc::sha256::hash(name)))
    , post_key(private_key_type::regenerate(fc::sha256::hash(std::string(name + "_post"))))
    , public_key(private_key.get_public_key())
{
}

Actor::Actor(const Actor& a)
{
    this->name = a.name;
    this->scr_amount = a.scr_amount;
    this->sp_amount = a.sp_amount;
    this->private_key = a.private_key;
    this->public_key = a.public_key;
    this->post_key = a.post_key;
}

Actor& Actor::scorum(asset scr)
{
    scr_amount = scr;
    return *this;
}

Actor& Actor::vests(asset vests)
{
    sp_amount = vests;
    return *this;
}
