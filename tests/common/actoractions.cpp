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

void ActorActions::create_account_by_committee(const Actor& a)
{
    account_create_by_committee_operation op;
    op.creator = _actor.name;
    op.new_account_name = a.name;
    op.owner = authority(1, a.public_key, 1);
    op.active = authority(1, a.public_key, 1);
    op.posting = authority(1, a.public_key, 1);
    op.memo_key = a.public_key;
    op.json_metadata = "";

    _chain.push_operation(op, _actor.private_key);
}

void ActorActions::transfer_to_scorumpower(const Actor& a, asset amount)
{
    _chain.transfer_to_scorumpower(_actor.name, a.name, amount);
}

void ActorActions::transfer(const Actor& a, asset amount)
{
    _chain.transfer(_actor.name, a.name, amount);
}

void ActorActions::give_scr(const Actor& a, long amount)
{
    transfer(a, ASSET_SCR(amount));
}

void ActorActions::give_sp(const Actor& a, long amount)
{
    transfer_to_scorumpower(a, ASSET_SCR(amount));
}

void ActorActions::create_budget(const std::string& json_metadata,
                                 asset balance,
                                 fc::time_point_sec start,
                                 fc::time_point_sec deadline)
{
    create_budget_operation op;
    op.owner = _actor.name;
    op.json_metadata = json_metadata;
    op.balance = balance;
    op.start = start;
    op.deadline = deadline;

    _chain.push_operation(op);
}

void ActorActions::delegate_sp_from_reg_pool(const Actor& a, asset amount)
{
    delegate_sp_from_reg_pool_operation op;
    op.delegatee = a.name;
    op.reg_committee_member = _actor.name;
    op.scorumpower = amount;

    _chain.push_operation(op);
}
