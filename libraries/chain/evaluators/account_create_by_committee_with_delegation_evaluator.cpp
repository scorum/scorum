#include <scorum/chain/evaluators/account_create_by_committee_with_delegation_evaluator.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/registration_objects.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

#include <fc/exception/exception.hpp>

namespace scorum {
namespace chain {
account_create_by_committee_with_delegation_evaluator::account_create_by_committee_with_delegation_evaluator(
    data_service_factory_i& svc_factory,
    account_service_i& account_svc,
    dba::db_accessor<registration_pool_object>& reg_pool_dba,
    dba::db_accessor<registration_committee_member_object>& reg_committee_dba,
    dba::db_accessor<reg_pool_sp_delegation_object>& reg_pool_delegation_dba)
    : evaluator_impl<data_service_factory_i, account_create_by_committee_with_delegation_evaluator>(svc_factory)
    , _account_svc(account_svc)
    , _reg_pool_dba(reg_pool_dba)
    , _reg_committee_dba(reg_committee_dba)
    , _reg_pool_delegation_dba(reg_pool_delegation_dba)
{
}

void account_create_by_committee_with_delegation_evaluator::do_apply(
    const protocol::account_create_by_committee_with_delegation_operation& op)
{
    _account_svc.check_account_existence(op.creator);
    _account_svc.check_account_existence(op.owner.account_auths);
    _account_svc.check_account_existence(op.active.account_auths);
    _account_svc.check_account_existence(op.posting.account_auths);

    FC_ASSERT(!_account_svc.is_exists(op.new_account_name), "Account ${0} already exists", ("0", op.new_account_name));
    FC_ASSERT(_reg_committee_dba.is_exists_by<by_account_name>(op.creator), "Account '${1}' is not committee member.",
              ("1", op.creator));
    FC_ASSERT(_reg_pool_dba.get().balance.amount >= op.delegation.amount, "Registration pool is exhausted.");

    const auto& new_acc = _account_svc.create_account(op.new_account_name, op.creator, op.memo_key, op.json_metadata,
                                                      op.owner, op.active, op.posting);

    _account_svc.increase_received_scorumpower(new_acc, op.delegation);
    _reg_pool_dba.update([&](registration_pool_object& pool) { pool.balance -= op.delegation; });

    _reg_pool_delegation_dba.create([&](reg_pool_sp_delegation_object& o) {
        o.delegatee = op.new_account_name;
        o.sp = op.delegation;
    });
}
}
}
