#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/services/account.hpp>

#include "database_default_integration.hpp"

namespace database_fixture {

class create_account_with_data_service_fixture : public database_default_integration_fixture
{
public:
    create_account_with_data_service_fixture()
        : data_service(db.obtain_service<dbs_account>())
        , user("user")
    {
    }

    void create_account()
    {
        data_service.create_account(user.name, initdelegate.name, user.public_key, "", authority(), authority(),
                                    authority(), asset(0, SCORUM_SYMBOL));
    }

    share_type calc_fee()
    {
        return std::max(db.get_dynamic_global_properties().median_chain_props.account_creation_fee.amount
                            * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER,
                        share_type(100));
    }

    dbs_account& data_service;

    const Actor user;
};

BOOST_FIXTURE_TEST_SUITE(create_account_with_data_service, create_account_with_data_service_fixture)

BOOST_AUTO_TEST_CASE(check_account_name)
{
    try
    {
        create_account();

        const account_object& acount = db.obtain_service<dbs_account>().get_account("user");

        BOOST_CHECK(acount.name == user.name);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_recovery_account_name)
{
    try
    {
        create_account();

        const account_object& acount = db.obtain_service<dbs_account>().get_account("user");

        BOOST_CHECK(acount.recovery_account == initdelegate.name);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(fail_on_second_creation)
{
    try
    {
        create_account();

        BOOST_CHECK_THROW(data_service.create_account(user.name, initdelegate.name, user.public_key, "", authority(),
                                                      authority(), authority(), asset(0, SCORUM_SYMBOL)),
                          std::logic_error);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_without_fee)
{
    try
    {
        const asset balance_before_creation = db.obtain_service<dbs_account>().get_account(initdelegate.name).balance;

        create_account();

        BOOST_CHECK(db.obtain_service<dbs_account>().get_account(initdelegate.name).balance == balance_before_creation);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_fee_after_creation)
{
    try
    {
        const asset balance_before_creation = db.obtain_service<dbs_account>().get_account(initdelegate.name).balance;

        const share_type fee = calc_fee();

        data_service.create_account(user.name, initdelegate.name, user.public_key, "", authority(), authority(),
                                    authority(), asset(fee, SCORUM_SYMBOL));

        BOOST_CHECK(db.obtain_service<dbs_account>().get_account(initdelegate.name).balance
                    == asset(balance_before_creation.amount - fee, SCORUM_SYMBOL));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // database_fixture

#endif
