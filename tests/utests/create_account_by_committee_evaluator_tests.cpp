#include <boost/test/unit_test.hpp>

#include <scorum/chain/evaluators/vote_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account_registration_bonus.hpp>

#include <scorum/chain/evaluators/registration_pool_evaluator.hpp>

#include "object_wrapper.hpp"
#include "defines.hpp"
#include <hippomocks.h>

namespace create_account_by_committee_evaluator_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

struct fixture : shared_memory_fixture
{
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    account_service_i* account_svc = mocks.Mock<account_service_i>();
    registration_pool_service_i* reg_pool_svc = mocks.Mock<registration_pool_service_i>();
    registration_committee_service_i* reg_committee_svc = mocks.Mock<registration_committee_service_i>();
    dynamic_global_property_service_i* dgp_svc = mocks.Mock<dynamic_global_property_service_i>();
    hardfork_property_service_i* hardfork_svc = mocks.Mock<hardfork_property_service_i>();
    account_registration_bonus_service_i* reg_bonus_svc = mocks.Mock<account_registration_bonus_service_i>();

    typedef void (account_service_i::*check_account_existence_by_name)(const account_name_type& a,
                                                                       const fc::optional<const char*>& b) const;

    typedef void (account_service_i::*check_account_existence_by_authority)(
        const account_authority_map&, const fc::optional<const char*>& context_type_name) const;

    void init()
    {
        mocks.OnCall(services, data_service_factory_i::account_registration_bonus_service).ReturnByRef(*reg_bonus_svc);
        mocks.OnCall(services, data_service_factory_i::dynamic_global_property_service).ReturnByRef(*dgp_svc);
        mocks.OnCall(services, data_service_factory_i::account_service).ReturnByRef(*account_svc);
        mocks.OnCall(services, data_service_factory_i::registration_pool_service).ReturnByRef(*reg_pool_svc);
        mocks.OnCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*reg_committee_svc);
        mocks.OnCall(services, data_service_factory_i::hardfork_property_service).ReturnByRef(*hardfork_svc);
    }
};

BOOST_FIXTURE_TEST_SUITE(registration_pool_evaluator_tests, fixture)

SCORUM_TEST_CASE(get_services_in_constructor)
{
    mocks.ExpectCall(services, data_service_factory_i::account_registration_bonus_service).ReturnByRef(*reg_bonus_svc);
    mocks.ExpectCall(services, data_service_factory_i::dynamic_global_property_service).ReturnByRef(*dgp_svc);
    mocks.ExpectCall(services, data_service_factory_i::account_service).ReturnByRef(*account_svc);
    mocks.ExpectCall(services, data_service_factory_i::registration_pool_service).ReturnByRef(*reg_pool_svc);
    mocks.ExpectCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*reg_committee_svc);
    mocks.ExpectCall(services, data_service_factory_i::hardfork_property_service).ReturnByRef(*hardfork_svc);

    registration_pool_evaluator evaluator(*services);
}

SCORUM_TEST_CASE(throw_exception_when_creator_is_not_in_committee)
{
    init();

    account_create_by_committee_operation op;
    op.creator = "alice";

    mocks.OnCallOverload(account_svc, (check_account_existence_by_name)&account_service_i::check_account_existence)
        .With(_, _);

    mocks.OnCallOverload(account_svc, (check_account_existence_by_authority)&account_service_i::check_account_existence)
        .With(_, _);

    mocks.ExpectCall(reg_committee_svc, registration_committee_i::is_exists).With("alice").Return(false);

    registration_pool_evaluator evaluator(*services);

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(create_account_with_zero_fee_on_hf_0_3_0)
{
    init();

    auto account = create_object<account_object>(shm, [](auto&) {});

    account_create_by_committee_operation op;
    op.creator = "alice";
    op.new_account_name = "bob";

    mocks.OnCallOverload(account_svc, (check_account_existence_by_name)&account_service_i::check_account_existence)
        .With(_, _);

    mocks.OnCallOverload(account_svc, (check_account_existence_by_authority)&account_service_i::check_account_existence)
        .With(_, _);

    mocks.OnCall(reg_committee_svc, registration_committee_i::is_exists).With(_).Return(true);

    mocks.ExpectCall(hardfork_svc, hardfork_property_service_i::has_hardfork).With(SCORUM_HARDFORK_0_3).Return(true);

    mocks.ExpectCall(account_svc, account_service_i::create_account)
        .With(op.new_account_name, op.creator, _, _, _, _, _, asset(0, SCORUM_SYMBOL))
        .ReturnByRef(account);

    registration_pool_evaluator evaluator(*services);
    evaluator.do_apply(op);
}

BOOST_AUTO_TEST_SUITE_END()
}
