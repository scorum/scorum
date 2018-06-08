#include <boost/test/unit_test.hpp>

#include <scorum/app/api_context.hpp>

#include <scorum/app/chain_api.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/witness_reward_in_sp_migration.hpp>
#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/reward_balancer_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/services/development_committee.hpp>

#include "database_trx_integration.hpp"

using namespace scorum;
using namespace scorum::app;
using namespace scorum::protocol;

namespace chain_api_tests {

struct chain_api_database_fixture : public database_fixture::database_integration_fixture
{
    api_context _api_ctx;
    chain_api _api_call;

    chain_api_database_fixture()
        : _api_ctx(app, API_CHAIN, std::make_shared<api_session_data>())
        , _api_call(_api_ctx)
    {
        static const asset registration_bonus = ASSET_SCR(100);
        genesis_state_type::registration_schedule_item single_stage{ 1u, 1u, 100u };
        genesis_state_type genesis = database_integration_fixture::default_genesis_state()
                                         .registration_supply(registration_bonus * 100)
                                         .registration_bonus(registration_bonus)
                                         .registration_schedule(single_stage)
                                         .committee(initdelegate.name)
                                         .generate();

        open_database(genesis);

        generate_block();
        validate_database();
    }
};

} // namespace chain_api_tests

BOOST_FIXTURE_TEST_SUITE(chain_api_tests, chain_api_tests::chain_api_database_fixture)

SCORUM_TEST_CASE(chain_properties_getter_test)
{
    const auto& dpo = db.obtain_service<chain::dbs_dynamic_global_property>().get();

    auto props = _api_call.get_chain_properties();

    BOOST_REQUIRE_EQUAL(props.chain_id, db.get_chain_id());
    BOOST_REQUIRE_EQUAL(props.head_block_id, dpo.head_block_id);
    BOOST_REQUIRE_EQUAL(props.head_block_number, dpo.head_block_number);
    BOOST_REQUIRE_EQUAL(props.last_irreversible_block_number, dpo.last_irreversible_block_num);
    BOOST_REQUIRE_EQUAL(props.current_aslot, dpo.current_aslot);
    BOOST_REQUIRE_EQUAL(props.current_witness, initdelegate.name);
    BOOST_REQUIRE_EQUAL(props.median_chain_props.account_creation_fee, dpo.median_chain_props.account_creation_fee);
    BOOST_REQUIRE_EQUAL(props.median_chain_props.maximum_block_size, dpo.median_chain_props.maximum_block_size);
    BOOST_REQUIRE(props.time == dpo.time);
    BOOST_REQUIRE(props.majority_version == dpo.majority_version);
    BOOST_REQUIRE(props.hf_version == db.obtain_service<chain::dbs_hardfork_property>().get().current_hardfork_version);
}

SCORUM_TEST_CASE(chain_properties_getter_after_generate_block_test)
{
    generate_block();

    auto props = _api_call.get_chain_properties();

    generate_block();

    auto props_new = _api_call.get_chain_properties();

    BOOST_REQUIRE_EQUAL(props_new.chain_id, props.chain_id);
    BOOST_REQUIRE_EQUAL(props_new.head_block_number, props.head_block_number + 1);
    BOOST_REQUIRE_EQUAL(props_new.last_irreversible_block_number, props.last_irreversible_block_number);
    BOOST_REQUIRE_EQUAL(props_new.current_aslot, props.current_aslot + 1);
    BOOST_REQUIRE_EQUAL(props_new.current_witness, initdelegate.name);
    BOOST_REQUIRE_EQUAL(props_new.median_chain_props.account_creation_fee,
                        props.median_chain_props.account_creation_fee);
    BOOST_REQUIRE_EQUAL(props_new.median_chain_props.maximum_block_size, props.median_chain_props.maximum_block_size);
    BOOST_REQUIRE_EQUAL(props_new.time.sec_since_epoch(), props.time.sec_since_epoch() + SCORUM_BLOCK_INTERVAL);
    BOOST_REQUIRE(props_new.majority_version == props.majority_version);
    BOOST_REQUIRE(props_new.hf_version == props.hf_version);
}

SCORUM_TEST_CASE(get_chain_capital_test)
{
    auto capital = _api_call.get_chain_capital();

    const auto& dpo = db.obtain_service<chain::dbs_dynamic_global_property>().get();

    BOOST_REQUIRE_EQUAL(capital.head_block_number, dpo.head_block_number);
    BOOST_REQUIRE_EQUAL(capital.head_block_id, dpo.head_block_id);
    BOOST_REQUIRE(capital.head_block_time == dpo.time);
    BOOST_REQUIRE_EQUAL(capital.current_witness, dpo.current_witness);

    BOOST_REQUIRE_EQUAL(capital.total_supply, dpo.total_supply);
    BOOST_REQUIRE_EQUAL(capital.circulating_capital, dpo.circulating_capital);
    BOOST_REQUIRE_EQUAL(capital.total_scorumpower, dpo.total_scorumpower);
    BOOST_REQUIRE_EQUAL(capital.total_witness_reward_scr, dpo.total_witness_reward_scr);
    BOOST_REQUIRE_EQUAL(capital.total_witness_reward_sp, dpo.total_witness_reward_sp);

    BOOST_REQUIRE_EQUAL(capital.registration_pool_balance,
                        db.obtain_service<chain::dbs_registration_pool>().get().balance);
    BOOST_REQUIRE_EQUAL(capital.fund_budget_balance, db.obtain_service<chain::dbs_budget>().get_fund_budget().balance);

    BOOST_REQUIRE_EQUAL(capital.dev_pool_scr_balance,
                        db.obtain_service<chain::dbs_development_committee>().get().scr_balance);
    BOOST_REQUIRE_EQUAL(capital.dev_pool_sp_balance,
                        db.obtain_service<chain::dbs_development_committee>().get().sp_balance);

    BOOST_REQUIRE_EQUAL(capital.content_balancer_scr, db.obtain_service<chain::dbs_content_reward_scr>().get().balance);
    BOOST_REQUIRE_EQUAL(capital.active_voters_balancer_scr,
                        db.obtain_service<chain::dbs_voters_reward_scr>().get().balance);
    BOOST_REQUIRE_EQUAL(capital.active_voters_balancer_sp,
                        db.obtain_service<chain::dbs_voters_reward_sp>().get().balance);

    BOOST_REQUIRE_EQUAL(capital.content_reward_fund_scr_balance,
                        db.obtain_service<chain::dbs_content_reward_fund_scr>().get().activity_reward_balance);
    BOOST_REQUIRE_EQUAL(capital.content_reward_fund_sp_balance,
                        db.obtain_service<chain::dbs_content_reward_fund_sp>().get().activity_reward_balance);
    BOOST_REQUIRE_EQUAL(capital.content_reward_fifa_world_cup_2018_bounty_fund_sp_balance, asset(0, SP_SYMBOL));

    BOOST_REQUIRE_EQUAL(capital.witness_reward_in_sp_migration_fund.amount,
                        db.obtain_service<chain::dbs_witness_reward_in_sp_migration>().get().balance);
}

SCORUM_TEST_CASE(get_reward_fund_test)
{
    auto reward = _api_call.get_reward_fund(reward_fund_type::content_reward_fund_sp);

    const auto& fund = db.obtain_service<chain::dbs_content_reward_fund_sp>().get();

    BOOST_REQUIRE_EQUAL(reward.activity_reward_balance, fund.activity_reward_balance);
    BOOST_REQUIRE(reward.recent_claims == fund.recent_claims);
    BOOST_REQUIRE(reward.last_update == fund.last_update);
    BOOST_REQUIRE(reward.author_reward_curve == fund.author_reward_curve);
    BOOST_REQUIRE(reward.curation_reward_curve == fund.curation_reward_curve);
}

BOOST_AUTO_TEST_SUITE_END()
