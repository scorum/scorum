#pragma once
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

namespace scorum {
namespace chain {

class registration_pool_object;
class registration_committee_member_object;
class reg_pool_sp_delegation_object;

namespace dba {
template <typename> class dba;
}
struct account_service_i;

class return_registration_bonus_evaluator
    : public evaluator_impl<data_service_factory_i, return_registration_bonus_evaluator>
{
public:
    using operation_type = protocol::return_registration_bonus_operation;

    return_registration_bonus_evaluator(data_service_factory_i&,
                                        account_service_i&,
                                        dba::db_accessor<registration_pool_object>&,
                                        dba::db_accessor<registration_committee_member_object>&,
                                        dba::db_accessor<reg_pool_sp_delegation_object>&);

    void do_apply(const protocol::return_registration_bonus_operation& op);

private:
    account_service_i& _account_svc;
    dba::db_accessor<registration_pool_object>& _reg_pool_dba;
    dba::db_accessor<registration_committee_member_object>& _reg_committee_dba;
    dba::db_accessor<reg_pool_sp_delegation_object>& _reg_pool_delegation_dba;
};
}
}
