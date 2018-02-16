#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include "database_integration.hpp"

#include "actor.hpp"
#include "genesis.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <sstream>

BOOST_AUTO_TEST_SUITE(genesis_db_tests)

#ifdef SRC_DIR
BOOST_FIXTURE_TEST_CASE(validate_src_json_test, scorum::chain::database_integration_fixture)
{
    boost::filesystem::path path_to_src_json(BOOST_PP_STRINGIZE(SRC_DIR));
    if (boost::filesystem::exists(path_to_src_json))
    {
        path_to_src_json /= ".."; // tests
        path_to_src_json /= ".."; // root
        path_to_src_json /= "genesis.json";

        path_to_src_json.normalize();

        ilog("Checking ${file}.", ("file", path_to_src_json.string()));

        boost::filesystem::ifstream fl;
        fl.open(path_to_src_json.string(), std::ios::in);

        BOOST_REQUIRE((bool)fl);

        std::stringstream ss;

        ss << fl.rdbuf();

        fl.close();

        genesis_state_type gs = fc::json::from_string(ss.str()).as<genesis_state_type>();

        BOOST_REQUIRE_NO_THROW(open_database(gs));
    }
}
#endif

struct genesis_base_test_fixture : public scorum::chain::database_integration_fixture
{
    genesis_base_test_fixture()
        : account_service(db.account_service())
        , initdelegate(TEST_INIT_DELEGATE_NAME)
    {
        genesis = Genesis::create();
    }

    void make_minimal_valid_genesis()
    {
        private_key_type init_delegate_priv_key
            = private_key_type::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
        public_key_type init_public_key = init_delegate_priv_key.get_public_key();
        asset as = ASSET_SCR(1e+3);
        initdelegate.public_key = init_public_key;
        initdelegate.scorum(as);

        asset rw = ASSET_SCR(1000e+9);
        genesis
            = Genesis::create().accounts_supply(as).rewards_supply(rw).accounts(initdelegate).witnesses(initdelegate);
    }

    scorum::chain::account_service_i& account_service;

    Genesis genesis;

    Actor initdelegate;
};

struct genesis_founders_test_fixture : public genesis_base_test_fixture
{
    genesis_founders_test_fixture()
        : bob("bob")
        , mike("mike")
        , luke("luke")
        , stiven("stiven")
    {
        make_minimal_valid_genesis();
    }

    Actor bob;
    Actor mike;
    Actor luke;
    Actor stiven;
};

BOOST_FIXTURE_TEST_CASE(founders_sp_distribution_test, genesis_founders_test_fixture)
{
    asset total_sp = ASSET_SP(1e+6);
    float total = 100.f;
    float pie = total / 2;

    bob.percent(pie);
    mike.percent(pie);

    genesis.founders(bob, mike).founders_supply(total_sp);

    BOOST_REQUIRE_NO_THROW(open_database(genesis.generate()));

    BOOST_CHECK_EQUAL(account_service.get_account(bob.name).vesting_shares, total_sp / 2);

    BOOST_CHECK_EQUAL(account_service.get_account(mike.name).vesting_shares, total_sp / 2);
}

BOOST_FIXTURE_TEST_CASE(founders_sp_distribution_with_pitiful_test, genesis_founders_test_fixture)
{
    asset total_sp = ASSET_SP(1e+6);
    float total = 100u;
    float pie = total / 2;
    float pitiful_pie = .5f / SCORUM_1_PERCENT; // 0.005 %

    bob.percent(pie);
    mike.percent(pie - pitiful_pie);
    luke.percent(pitiful_pie / 2);
    stiven.percent(pitiful_pie / 2); // last pitiful will get some SP,
    // give each founders more than 0.01 % (depend on SCORUM_1_PERCENT value) if you want equity

    genesis.founders(bob, mike, luke, stiven).founders_supply(total_sp);

    BOOST_REQUIRE_NO_THROW(open_database(genesis.generate()));

    BOOST_CHECK_EQUAL(account_service.get_account(bob.name).vesting_shares, total_sp / 2);

    BOOST_CHECK_GT(account_service.get_account(mike.name).vesting_shares, ASSET_NULL_SP);

    BOOST_CHECK_EQUAL(account_service.get_account(luke.name).vesting_shares, ASSET_NULL_SP);

    BOOST_CHECK_GT(account_service.get_account(stiven.name).vesting_shares, ASSET_NULL_SP);
}

struct genesis_registration_bonus_test_fixture : public genesis_base_test_fixture
{
    genesis_registration_bonus_test_fixture()
        : registration_pool_service(db.registration_pool_service())
    {
        for (size_t ci = 0; ci < sz_investors; ++ci)
        {
            std::stringstream name;
            name << "Mr. " << ci;
            investors[ci] = name.str();
        }
        make_minimal_valid_genesis();

        for (size_t ci = 0; ci < sz_investors; ++ci)
        {
            genesis.account_create(investors[ci]);
        }
    }

    scorum::chain::registration_pool_service_i& registration_pool_service;

    static const size_t sz_investors = 10u;
    Actor investors[sz_investors];
};

BOOST_FIXTURE_TEST_CASE(registration_bonus_distribution_test, genesis_registration_bonus_test_fixture)
{
    asset rs = ASSET_SCR(1e+9);
    uint32_t luckies = sz_investors + 1; //+ initdelegate
    uint32_t users = 100u;
    registration_stage single_stage{ 1u, users, 100u };
    asset bonus = rs / luckies;
    genesis.registration_supply(rs)
        .registration_bonus(bonus)
        .registration_schedule(single_stage)
        .committee(initdelegate);

    BOOST_REQUIRE_NO_THROW(open_database(genesis.generate()));

    BOOST_CHECK_EQUAL(account_service.get_account(investors[0].name).vesting_shares, ASSET_SP(bonus.amount.value));

    BOOST_CHECK_EQUAL(account_service.get_account(investors[sz_investors - 1].name).vesting_shares,
                      ASSET_SP(bonus.amount.value));

    BOOST_CHECK_EQUAL(registration_pool_service.get().balance, rs - bonus * luckies);
}

BOOST_FIXTURE_TEST_CASE(registration_bonus_downgrade_distribution_test, genesis_registration_bonus_test_fixture)
{
    asset rs = ASSET_SCR(1e+9);
    uint32_t luckies = sz_investors + 1; //+ initdelegate
    registration_stage stage1{ 1u, sz_investors / 2, 100u };
    registration_stage stage2{ 2u, sz_investors / 2, 50u };
    asset bonus = rs / luckies;
    genesis.registration_supply(rs)
        .registration_bonus(bonus)
        .registration_schedule(stage1, stage2)
        .committee(initdelegate);

    BOOST_REQUIRE_NO_THROW(open_database(genesis.generate()));

    BOOST_CHECK_EQUAL(account_service.get_account(investors[0].name).vesting_shares, ASSET_SP(bonus.amount.value));

    BOOST_CHECK_EQUAL(account_service.get_account(investors[sz_investors - 1].name).vesting_shares,
                      ASSET_SP(bonus.amount.value / 2));
}

BOOST_FIXTURE_TEST_CASE(registration_bonus_downgrade_exhaust_distribution_test, genesis_registration_bonus_test_fixture)
{
    asset rs = ASSET_SCR(1e+9);
    uint32_t luckies = sz_investors + 1; //+ initdelegate
    registration_stage stage1{ 1u, sz_investors / 2, 100u };
    registration_stage stage2{ 2u, sz_investors / 2, 50u };
    asset bonus = rs / luckies;
    bonus *= 2;
    genesis.registration_supply(rs)
        .registration_bonus(bonus)
        .registration_schedule(stage1, stage2)
        .committee(initdelegate);

    BOOST_REQUIRE_NO_THROW(open_database(genesis.generate()));

    BOOST_CHECK_EQUAL(account_service.get_account(investors[0].name).vesting_shares, ASSET_SP(bonus.amount.value));

    BOOST_CHECK_EQUAL(account_service.get_account(investors[sz_investors - 1].name).vesting_shares, ASSET_NULL_SP);
}

struct steemit_bounty_test_fixture : public genesis_base_test_fixture
{
    steemit_bounty_test_fixture()
        : bob("bob")
        , luke("luke")
        , stiven("stiven")
    {
        make_minimal_valid_genesis();
    }

    Actor bob;
    Actor luke;
    Actor stiven;
};

BOOST_FIXTURE_TEST_CASE(steemit_bounty_distribution_test, steemit_bounty_test_fixture)
{
    asset total_sp = ASSET_SP(1e+6);
    asset pie = total_sp / 4;

    bob.vests(pie);
    luke.vests(pie);
    stiven.vests(total_sp - pie * 2);

    genesis.steemit_bounty_accounts(bob, luke, stiven).steemit_bounty_accounts_supply(total_sp);

    BOOST_REQUIRE_NO_THROW(open_database(genesis.generate()));

    BOOST_CHECK_EQUAL(account_service.get_account(bob.name).vesting_shares, pie);

    BOOST_CHECK_EQUAL(account_service.get_account(luke.name).vesting_shares, pie);

    BOOST_CHECK_EQUAL(account_service.get_account(stiven.name).vesting_shares, total_sp - pie * 2);
}

struct dev_poll_test_fixture : public genesis_base_test_fixture
{
    dev_poll_test_fixture()
        : dev_pool_service(db.dev_pool_service())
    {
        make_minimal_valid_genesis();
    }

    scorum::chain::dev_pool_service_i& dev_pool_service;
};

BOOST_FIXTURE_TEST_CASE(dev_pool_test, dev_poll_test_fixture)
{
    using scorum::protocol::account_name_type;

    asset dev_sp = ASSET_SP(1e+6);

    genesis.dev_supply(dev_sp);

    BOOST_REQUIRE_NO_THROW(open_database(genesis.generate()));

    BOOST_REQUIRE_NO_THROW(account_service.check_account_existence(SCORUM_DEV_POOL_SP_LOCKED_ACCOUNT));

    BOOST_REQUIRE(dev_pool_service.is_exists());
}

BOOST_AUTO_TEST_SUITE_END()

#endif
