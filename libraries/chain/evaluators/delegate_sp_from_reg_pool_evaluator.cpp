#include <scorum/chain/evaluators/delegate_sp_from_reg_pool_evaluator.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/registration_objects.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

#include <fc/exception/exception.hpp>

namespace scorum {
namespace chain {
delegate_sp_from_reg_pool_evaluator::delegate_sp_from_reg_pool_evaluator(
    data_service_factory_i& svc_factory,
    account_service_i& account_svc,
    dba::db_accessor<registration_pool_object>& reg_pool_dba,
    dba::db_accessor<registration_committee_member_object>& reg_committee_dba,
    dba::db_accessor<reg_pool_sp_delegation_object>& reg_pool_delegation_dba)
    : evaluator_impl<data_service_factory_i, delegate_sp_from_reg_pool_evaluator>(svc_factory)
    , _account_svc(account_svc)
    , _reg_pool_dba(reg_pool_dba)
    , _reg_committee_dba(reg_committee_dba)
    , _reg_pool_delegation_dba(reg_pool_delegation_dba)
{
}

void delegate_sp_from_reg_pool_evaluator::do_apply(const protocol::delegate_sp_from_reg_pool_operation& op)
{
    _account_svc.check_account_existence(op.reg_committee_member);
    _account_svc.check_account_existence(op.delegatee);

    FC_ASSERT(_reg_committee_dba.is_exists_by<by_account_name>(op.reg_committee_member),
              "Account '${1}' is not committee member.", ("1", op.reg_committee_member));

    auto is_delegation_exists = _reg_pool_delegation_dba.is_exists_by<by_delegatee>(op.delegatee);
    if (!is_delegation_exists)
    {
        FC_ASSERT(op.scorumpower.amount > 0, "Account has no delegated SP from registration pool");
        FC_ASSERT(_reg_pool_dba.get().balance.amount >= op.scorumpower.amount, "Registration pool is exhausted.");

        _account_svc.increase_received_scorumpower(op.delegatee, op.scorumpower);
        _reg_pool_dba.update([&](registration_pool_object& pool) { pool.balance -= op.scorumpower; });

        _reg_pool_delegation_dba.create([&](reg_pool_sp_delegation_object& o) {
            o.delegatee = op.delegatee;
            o.sp = op.scorumpower;
        });
    }
    else
    {
        const auto& delegation = _reg_pool_delegation_dba.get_by<by_delegatee>(op.delegatee);
        auto extra_sp = op.scorumpower - delegation.sp;

        FC_ASSERT(_reg_pool_dba.get().balance.amount >= extra_sp.amount, "Registration pool is exhausted.");

        _account_svc.increase_received_scorumpower(op.delegatee, extra_sp);
        _reg_pool_dba.update([&](registration_pool_object& pool) { pool.balance -= extra_sp; });

        if (op.scorumpower.amount > 0)
            _reg_pool_delegation_dba.update(delegation, [&](reg_pool_sp_delegation_object& o) { o.sp += extra_sp; });
        else
            _reg_pool_delegation_dba.remove(delegation);
    }
}
}
}
