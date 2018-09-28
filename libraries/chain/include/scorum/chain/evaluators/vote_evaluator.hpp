#pragma once

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

class data_service_factory_i;
class account_service_i;
class comment_service_i;
class dynamic_global_property_service_i;
class comment_vote_service_i;
class hardfork_property_service_i;

class vote_evaluator : public evaluator_impl<data_service_factory_i, vote_evaluator>
{
public:
    using operation_type = scorum::protocol::vote_operation;

    vote_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);

    protocol::vote_weight_type get_weigth(const operation_type& o) const;

private:
    account_service_i& _account_service;
    comment_service_i& _comment_service;
    comment_vote_service_i& _comment_vote_service;
    dynamic_global_property_service_i& _dgp_service;
    hardfork_property_service_i& _hardfork_service;
};

} // namespace chain
} // namespace scorum
