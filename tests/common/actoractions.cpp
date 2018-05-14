#include "actoractions.hpp"
#include "database_trx_integration.hpp"

#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/protocol/scorum_operations.hpp>

using namespace scorum::chain;

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
    transfer_to_scorumpower(a, ASSET_SCR(amount));
}

void ActorActions::vote(const std::string& author, const std::string& permlink, int16_t weight)
{
    auto& comment_service = _chain.db.obtain_service<dbs_comment>();

    const comment_object* comment = comment_service.find_by<by_permlink>(boost::make_tuple(author, permlink));

    if (comment == nullptr)
    {
        FC_THROW("no such comment in db");
    }

    vote_operation op;
    op.voter = _actor.name;
    op.author = author;
    op.permlink = permlink;
    op.weight = weight;

    _chain.push_operation(op);
}

void ActorActions::create_budget(const std::string& permlink, asset balance, fc::time_point_sec deadline)
{
    create_budget_operation op;
    op.owner = _actor.name;
    op.content_permlink = permlink;
    op.balance = balance;
    op.deadline = deadline;

    _chain.push_operation(op);
}
