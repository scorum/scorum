#include <boost/test/unit_test.hpp>

#include <scorum/protocol/proposal_operations.hpp>
#include <scorum/chain/evaluators/evaluator_registry.hpp>
#include <scorum/chain/evaluators/proposal_evaluators.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <hippomocks.h>

BOOST_AUTO_TEST_CASE(test_proposal_operation_register)
{
    using namespace scorum::chain;
    using namespace scorum::protocol;

    MockRepository mocks;
    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();
    registration_committee_service_i* committee = mocks.Mock<registration_committee_service_i>();

    mocks.ExpectCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*committee);
    mocks.ExpectCall(committee, committee_i::add_member).With("alice");

    scorum::chain::evaluator_registry<proposal_operation> evaluators(*services);
    evaluators.register_evaluator<registration_committee::proposal_add_member_evaluator>();

    registration_committee_add_member_operation op;
    op.account_name = "alice";

    proposal_operation operation = op;

    auto& evaluator = evaluators.get_evaluator(operation);

    evaluator.apply(operation);
}
