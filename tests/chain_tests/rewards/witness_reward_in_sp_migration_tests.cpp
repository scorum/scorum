#include <boost/test/unit_test.hpp>

#include "database_trx_integration.hpp"

#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/chain/database/block_tasks/process_witness_reward_in_sp_migration.hpp>

#include <scorum/protocol/config.hpp>
#include <boost/make_unique.hpp>

#include "actor.hpp"
#include "genesis.hpp"

using namespace scorum::chain;

namespace witness_reward_in_sp_migration_tests {

using reward_map_type = std::map<account_name_type, asset>;

struct witness_reward_visitor
{
    typedef void result_type;

    account_name_type last_producer;
    reward_map_type reward_map;

    asset get_last_reward()
    {
        if (reward_map.empty())
        {
            return asset();
        }

        return reward_map[last_producer];
    }

    void operator()(const producer_reward_operation& op)
    {
        last_producer = op.producer;
        reward_map.insert(std::make_pair(last_producer, op.reward));
    }

    template <typename Op> void operator()(Op&&) const
    {
    }
};

struct witness_reward_in_sp_migration_fixture : public database_fixture::database_trx_integration_fixture
{
    witness_reward_in_sp_migration_fixture()
        : budget_service(db.budget_service())
        , dynamic_global_property_service(db.dynamic_global_property_service())
        , old_reward_alg_switch_reward_block_num(
              database_ns::process_witness_reward_in_sp_migration::old_reward_alg_switch_reward_block_num)
        , new_reward_to_migrate(database_ns::process_witness_reward_in_sp_migration::new_reward_to_migrate)
        , old_reward_to_migrate(database_ns::process_witness_reward_in_sp_migration::old_reward_to_migrate)
    {
        genesis_state_type genesis;

        genesis = Genesis::create()
                      .accounts_supply(TEST_ACCOUNTS_INITIAL_SUPPLY)
                      .rewards_supply(ASSET_SCR(4800000e+9))
                      .witnesses(initdelegate)
                      .dev_committee(initdelegate)
                      .generate(fc::time_point_sec::from_iso_string("2018-03-23T14:15:00"));

        // to set per block reward like in mainnet
        scorum::protocol::detail::override_config(boost::make_unique<scorum::protocol::detail::config>());

        open_database(genesis);

        scorum::protocol::detail::override_config(
            boost::make_unique<scorum::protocol::detail::config>(scorum::protocol::detail::config::test));

        const auto& fund_budget = budget_service.get_fund_budget();
        asset initial_per_block_reward = fund_budget.per_block;

        asset witness_reward = initial_per_block_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;

        BOOST_REQUIRE_EQUAL(witness_reward.amount, new_reward_to_migrate);

        db.post_apply_operation.connect([&](const operation_notification& note) { note.op.visit(reward_visitor); });
    }

    budget_service_i& budget_service;
    dynamic_global_property_service_i& dynamic_global_property_service;

    witness_reward_visitor reward_visitor;

    const uint32_t old_reward_alg_switch_reward_block_num;
    const share_type new_reward_to_migrate;
    const share_type old_reward_to_migrate;

    //    actors_vector_type witnesses;
};

BOOST_FIXTURE_TEST_SUITE(witness_reward_in_sp_migration_tests, witness_reward_in_sp_migration_fixture)

BOOST_AUTO_TEST_CASE(rest_of_reward_hold_check)
{
    BOOST_REQUIRE_LT(dynamic_global_property_service.get().head_block_number, old_reward_alg_switch_reward_block_num);

    generate_block();

    BOOST_REQUIRE_EQUAL(reward_visitor.get_last_reward(), asset(new_reward_to_migrate, SP_SYMBOL));

    generate_blocks(old_reward_alg_switch_reward_block_num);

    BOOST_REQUIRE_EQUAL(reward_visitor.get_last_reward(), asset(old_reward_to_migrate, SP_SYMBOL));
}

BOOST_AUTO_TEST_CASE(rest_of_reward_distribution_check)
{
    generate_blocks(SCORUM_WITNESS_REWARD_MIGRATION_DATE - SCORUM_BLOCK_INTERVAL);

    BOOST_REQUIRE_EQUAL(reward_visitor.get_last_reward(), asset(old_reward_to_migrate, SP_SYMBOL));

    reward_map_type old_reward_map = reward_visitor.reward_map;

    generate_block();

    auto current_witness = dynamic_global_property_service.get().current_witness;

    BOOST_REQUIRE(reward_visitor.reward_map.find(current_witness) != reward_visitor.reward_map.end());

    if (old_reward_map.find(current_witness) == old_reward_map.end())
    {
        old_reward_map[current_witness] = asset(old_reward_to_migrate, SP_SYMBOL);
    }

    for (const auto& reward_info : reward_visitor.reward_map)
    {
        asset new_reward = reward_info.second;
        asset old_reward = old_reward_map[reward_info.first];
        BOOST_REQUIRE_GT(new_reward, old_reward);
        BOOST_REQUIRE_EQUAL(new_reward - old_reward, old_reward * 5 / 100);
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
