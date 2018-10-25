#include <scorum/chain/evaluators/return_registration_bonus_evaluator.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/registration_objects.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

#include <fc/exception/exception.hpp>

namespace scorum {
namespace chain {
return_registration_bonus_evaluator::return_registration_bonus_evaluator(
    data_service_factory_i& svc_factory,
    account_service_i& account_svc,
    dba::db_accessor<registration_pool_object>& reg_pool_dba,
    dba::db_accessor<registration_committee_member_object>& reg_committee_dba,
    dba::db_accessor<reg_pool_sp_delegation_object>& reg_pool_delegation_dba)
    : evaluator_impl<data_service_factory_i, return_registration_bonus_evaluator>(svc_factory)
    , _account_svc(account_svc)
    , _reg_pool_dba(reg_pool_dba)
    , _reg_committee_dba(reg_committee_dba)
    , _reg_pool_delegation_dba(reg_pool_delegation_dba)
{
}

void return_registration_bonus_evaluator::do_apply(const protocol::return_registration_bonus_operation& op)
{
    _account_svc.check_account_existence(op.reg_committee_member);
    _account_svc.check_account_existence(op.account_name);

    FC_ASSERT(_reg_committee_dba.is_exists_by<by_account_name>(op.reg_committee_member),
              "Account '${1}' is not committee member.", ("1", op.reg_committee_member));

    FC_ASSERT(_reg_pool_delegation_dba.is_exists_by<by_delegatee>(op.account_name),
              "Account '${1}' already returned his registration bonus.", ("1", op.account_name));

    const auto& delegation = _reg_pool_delegation_dba.get_by<by_delegatee>(op.account_name);

    _account_svc.decrease_received_scorumpower(op.account_name, delegation.sp);
    _reg_pool_dba.update([&](registration_pool_object& pool) { pool.balance += delegation.sp; });
    _reg_pool_delegation_dba.remove(delegation);
}
}
}
