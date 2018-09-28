#include <boost/test/unit_test.hpp>

#include <scorum/chain/evaluators/vote_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include "object_wrapper.hpp"
#include <hippomocks.h>

namespace vote_evaluator_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

struct fixture
{
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    account_service_i* account_service = mocks.Mock<account_service_i>();
    comment_service_i* comment_service = mocks.Mock<comment_service_i>();
    comment_vote_service_i* comment_vote_service = mocks.Mock<comment_vote_service_i>();
    hardfork_property_service_i* hardfork_service = mocks.Mock<hardfork_property_service_i>();
    dynamic_global_property_service_i* dgp_service = mocks.Mock<dynamic_global_property_service_i>();

    void init()
    {
        mocks.OnCall(services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.OnCall(services, data_service_factory_i::comment_service).ReturnByRef(*comment_service);
        mocks.OnCall(services, data_service_factory_i::comment_vote_service).ReturnByRef(*comment_vote_service);
        mocks.OnCall(services, data_service_factory_i::hardfork_property_service).ReturnByRef(*hardfork_service);
        mocks.OnCall(services, data_service_factory_i::dynamic_global_property_service).ReturnByRef(*dgp_service);
    }

    const int16_t min_weigth_hf_0_0 = -100;
    const int16_t max_weigth_hf_0_0 = 100;

    const int16_t min_weigth_hf_0_2 = -1 * SCORUM_PERCENT(100);
    const int16_t max_weigth_hf_0_2 = SCORUM_PERCENT(100);
};

BOOST_FIXTURE_TEST_SUITE(vote_evaluator_tests, fixture)

BOOST_AUTO_TEST_CASE(get_services_in_constructor)
{
    mocks.ExpectCall(services, data_service_factory_i::account_service).ReturnByRef(*account_service);
    mocks.ExpectCall(services, data_service_factory_i::comment_service).ReturnByRef(*comment_service);
    mocks.ExpectCall(services, data_service_factory_i::comment_vote_service).ReturnByRef(*comment_vote_service);
    mocks.ExpectCall(services, data_service_factory_i::dynamic_global_property_service).ReturnByRef(*dgp_service);
    mocks.ExpectCall(services, data_service_factory_i::hardfork_property_service).ReturnByRef(*hardfork_service);

    vote_evaluator evaluator(*services);
}

BOOST_AUTO_TEST_CASE(throw_assert_when_weight_is_not_in_range_hf_0_0)
{
    init();

    vote_evaluator evaluator(*services);

    {
        mocks.ExpectCall(hardfork_service, hardfork_property_service_i::has_hardfork)
            .With(SCORUM_HARDFORK_0_2)
            .Return(false);

        vote_operation op;
        op.weight = min_weigth_hf_0_0 - 1;

        BOOST_CHECK_THROW(evaluator.get_weigth(op), fc::assert_exception);
    }

    {
        mocks.ExpectCall(hardfork_service, hardfork_property_service_i::has_hardfork)
            .With(SCORUM_HARDFORK_0_2)
            .Return(false);

        vote_operation op;
        op.weight = max_weigth_hf_0_0 + 1;

        BOOST_CHECK_THROW(evaluator.get_weigth(op), fc::assert_exception);
    }
}

BOOST_AUTO_TEST_CASE(throw_assert_when_weight_is_not_in_range_hf_0_2)
{
    init();

    vote_evaluator evaluator(*services);

    {
        mocks.ExpectCall(hardfork_service, hardfork_property_service_i::has_hardfork)
            .With(SCORUM_HARDFORK_0_2)
            .Return(false);

        vote_operation op;
        op.weight = min_weigth_hf_0_2 - 1;

        BOOST_CHECK_THROW(evaluator.get_weigth(op), fc::assert_exception);
    }

    {
        mocks.ExpectCall(hardfork_service, hardfork_property_service_i::has_hardfork)
            .With(SCORUM_HARDFORK_0_2)
            .Return(false);

        vote_operation op;
        op.weight = max_weigth_hf_0_2 + 1;

        BOOST_CHECK_THROW(evaluator.get_weigth(op), fc::assert_exception);
    }
}

BOOST_AUTO_TEST_CASE(dont_throw_exception_when_weigth_in_range_hf_0_0)
{
    init();

    vote_evaluator evaluator(*services);

    for (int16_t w = min_weigth_hf_0_0; w < max_weigth_hf_0_0; w++)
    {
        mocks.ExpectCall(hardfork_service, hardfork_property_service_i::has_hardfork)
            .With(SCORUM_HARDFORK_0_2)
            .Return(false);

        vote_operation op;
        op.weight = w;

        BOOST_CHECK_NO_THROW(evaluator.get_weigth(op));
    }
}

BOOST_AUTO_TEST_CASE(dont_throw_exception_when_weigth_in_range_hf_0_2)
{
    init();

    vote_evaluator evaluator(*services);

    for (int16_t w = min_weigth_hf_0_2; w < max_weigth_hf_0_2; w++)
    {
        mocks.ExpectCall(hardfork_service, hardfork_property_service_i::has_hardfork)
            .With(SCORUM_HARDFORK_0_2)
            .Return(true);

        vote_operation op;
        op.weight = w;

        BOOST_CHECK_NO_THROW(evaluator.get_weigth(op));
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
