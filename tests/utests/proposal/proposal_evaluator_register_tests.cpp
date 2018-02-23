#include <boost/test/unit_test.hpp>

#include <scorum/protocol/proposal_operations.hpp>
#include <scorum/chain/evaluators/evaluator_registry.hpp>
#include <scorum/chain/evaluators/proposal_operations_evaluators.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <hippomocks.h>

using namespace scorum::chain;
using namespace scorum::protocol;

BOOST_AUTO_TEST_CASE(test_xxx)
{
    MockRepository mocks;
    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    scorum::chain::evaluator_registry<proposal_operation> reg(*services);
    reg.register_evaluator<proposal_add_member_evaluator<registration_committee_add_member_operation>>();
    //    reg.register_evaluator<set_quorum_evaluator<registration_committee_add_member_operation>>();

    registration_committee_add_member_operation op;
    op.account_name = "alice";

    proposal_operation operation = op;

    auto& evaluator = reg.get_evaluator(operation);

    evaluator.apply(operation);
}
