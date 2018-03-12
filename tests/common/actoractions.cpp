#include "actoractions.hpp"
#include "database_trx_integration.hpp"

ActorActions::ActorActions(chain_type& chain, const Actor& a)
    : _chain(chain)
    , _actor(a)
{
}

void ActorActions::create_account(const Actor& a)
{
    _chain.account_create(a.name, _actor.name, _actor.private_key, _chain.get_account_creation_fee(), a.public_key,
                          a.post_key.get_public_key(), "");
}

void ActorActions::transfer_to_scorumpower(const Actor& a, asset amount)
{
    _chain.transfer_to_scorumpower(_actor.name, a.name, amount);
}

void ActorActions::transfer(const Actor& a, asset amount)
{
    _chain.transfer(_actor.name, a.name, amount);
}

void ActorActions::give_scr(const Actor& a, int amount)
{
    transfer(a, ASSET_SCR(amount));
}

void ActorActions::give_sp(const Actor& a, int amount)
{
    transfer_to_vest(a, ASSET_SP(amount));
}
