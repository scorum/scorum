#pragma once
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

class registration_pool_object;
class registration_committee_member_object;
class reg_pool_sp_delegation_object;

namespace dba {
template <typename> class db_accessor;
}
struct account_service_i;

class delegate_sp_from_reg_pool_evaluator
    : public evaluator_impl<data_service_factory_i, delegate_sp_from_reg_pool_evaluator>
{
public:
    using operation_type = protocol::delegate_sp_from_reg_pool_operation;

    delegate_sp_from_reg_pool_evaluator(data_service_factory_i&,
                                        account_service_i&,
                                        dba::db_accessor<registration_pool_object>&,
                                        dba::db_accessor<registration_committee_member_object>&,
                                        dba::db_accessor<reg_pool_sp_delegation_object>&);

    void do_apply(const protocol::delegate_sp_from_reg_pool_operation& op);

private:
    account_service_i& _account_svc;
    dba::db_accessor<registration_pool_object>& _reg_pool_dba;
    dba::db_accessor<registration_committee_member_object>& _reg_committee_dba;
    dba::db_accessor<reg_pool_sp_delegation_object>& _reg_pool_delegation_dba;
};
}
}
