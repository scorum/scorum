#include "actoractions.hpp"
#include "database_trx_integration.hpp"

ActorActions::ActorActions(database_trx_integration_fixture& fix, const Actor& a)
    : _f(fix)
    , _actor(a)
{
}

void ActorActions::create()
{
    _f.account_create(_actor.name, _actor.public_key, _actor.post_key.get_public_key());
}

void ActorActions::transfer_to_vest(const Actor& a, asset amount)
{
    _f.transfer_to_vest(_actor.name, a.name, amount);
}

void ActorActions::transfer(const Actor& a, asset amount)
{
    _f.transfer(_actor.name, a.name, amount);
}

void ActorActions::give_scr(const Actor& a, int amount)
{
    transfer(a, ASSET_SCR(amount));
}
