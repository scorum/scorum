#include <boost/test/unit_test.hpp>

#include <scorum/protocol/proposal_operations.hpp>
//#include <scorum/chain/committee_factory.hpp>
#include <scorum/chain/evaluators/evaluator_registry.hpp>

#include <hippomocks.h>

using namespace scorum::chain;
using namespace scorum::protocol;

struct get_committee_quorum
{
    virtual percent_type operator()() = 0;
};

struct quorum_evaluator : public get_committee_quorum
{
    percent_type operator()() override
    {
        return 0u;
    }
};

BOOST_AUTO_TEST_CASE(test_xxx)
{
    //    MockRepository mocks;
    //    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    //    committee_factory factory(*services);

    //    scorum::chain::evaluator_registry<proposal_operation, committee_factory> reg(factory);

    //    std::vector<std::function>
}
