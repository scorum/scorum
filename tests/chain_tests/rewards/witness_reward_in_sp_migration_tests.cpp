#include <boost/test/unit_test.hpp>

#include "database_trx_integration.hpp"

#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/witness_reward_in_sp_migration.hpp>

#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/chain/database/block_tasks/process_witness_reward_in_sp_migration.hpp>

#include <scorum/protocol/config.hpp>
#include <boost/make_unique.hpp>

#include <map>
#include <vector>
#include <math.h>

#include <boost/range/numeric.hpp>

#include "actor.hpp"
#include "genesis.hpp"

using namespace scorum::chain;

namespace witness_reward_in_sp_migration_tests {

struct witness_reward_info
{
    witness_reward_info(uint32_t reward_block_num_, const asset& reward_)
        : reward_block_num(reward_block_num_)
        , reward(reward_)
    {
    }
    uint32_t reward_block_num = 0u;
    asset reward;
};

using witness_reward_infos_type = std::vector<witness_reward_info>;
using reward_map_type = std::map<account_name_type, witness_reward_infos_type>;

struct witness_reward_visitor
{
    typedef void result_type;

    witness_reward_visitor(database& db)
        : _db(db)
    {
    }

    account_name_type last_producer;
    reward_map_type reward_map;

    asset get_last_reward()
    {
        if (reward_map.empty())
        {
            return asset();
        }

        return (*reward_map[last_producer].rbegin()).reward;
    }

    void operator()(const producer_reward_operation& op)
    {
        last_producer = op.producer;
        witness_reward_infos_type& vec = reward_map[last_producer];
        vec.push_back({ _db.head_block_num(), op.reward });
    }

    template <typename Op> void operator()(Op&&) const
    {
    }

private:
    database& _db;
};

struct witness_reward_in_sp_migration_fixture : public database_fixture::database_trx_integration_fixture
{
    witness_reward_in_sp_migration_fixture()
        : budget_service(db.budget_service())
        , dynamic_global_property_service(db.dynamic_global_property_service())
        , witness_reward_in_sp_migration_service(db.witness_reward_in_sp_migration_service())
        , reward_visitor(db)
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
    witness_reward_in_sp_migration_service_i& witness_reward_in_sp_migration_service;

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
    generate_blocks(old_reward_alg_switch_reward_block_num);

    uint32_t block_till_migration
        = ((SCORUM_WITNESS_REWARD_MIGRATION_DATE - SCORUM_BLOCK_INTERVAL) - db.head_block_time()).to_seconds()
        / SCORUM_BLOCK_INTERVAL;

    // we must walk through more than one round to increase reward more then one per block for each witness
    BOOST_REQUIRE_GE(block_till_migration, (uint32_t)(SCORUM_MAX_WITNESSES * SCORUM_MAX_WITNESSES));

    generate_blocks((uint32_t)(SCORUM_MAX_WITNESSES * SCORUM_MAX_WITNESSES));

    // do not miss blocks
    generate_blocks(SCORUM_WITNESS_REWARD_MIGRATION_DATE - SCORUM_BLOCK_INTERVAL);

    BOOST_REQUIRE_EQUAL(reward_visitor.get_last_reward(), asset(old_reward_to_migrate, SP_SYMBOL));

    using total_reward_map_type = std::map<account_name_type, share_type>;
    total_reward_map_type old_total_reward_map;
    total_reward_map_type new_total_reward_map;

    auto lbcalc_total_reward = [&](total_reward_map_type& total_reward_map, const reward_map_type::value_type& it) {
        for (const witness_reward_info& info : it.second)
        {
            if (info.reward_block_num > old_reward_alg_switch_reward_block_num)
            {
                total_reward_map[it.first] += info.reward.amount;
            }
        }
        return total_reward_map;
    };

    old_total_reward_map = std::accumulate(reward_visitor.reward_map.begin(), reward_visitor.reward_map.end(),
                                           old_total_reward_map, lbcalc_total_reward);

    BOOST_CHECK_EQUAL(witness_reward_in_sp_migration_service.is_exists(), true);

    // migration cashout for witnesses
    generate_block();

    BOOST_CHECK_EQUAL(witness_reward_in_sp_migration_service.is_exists(), false);

    auto current_witness = dynamic_global_property_service.get().current_witness;

    BOOST_REQUIRE(reward_visitor.reward_map.find(current_witness) != reward_visitor.reward_map.end());

    old_total_reward_map[current_witness] += old_reward_to_migrate;

    new_total_reward_map = std::accumulate(reward_visitor.reward_map.begin(), reward_visitor.reward_map.end(),
                                           new_total_reward_map, lbcalc_total_reward);

    for (const auto& reward_info : new_total_reward_map)
    {
        share_type new_reward = reward_info.second;
        share_type old_reward = old_total_reward_map[reward_info.first];
        BOOST_REQUIRE_GT(new_reward, old_reward);
        // check if the difference does not exceed the precision of integer rounding (for 1e+9 it is 100)
        BOOST_CHECK_LE((new_reward - old_reward).value - new_reward.value * 5 / 100, 100);
    }

    generate_block();

    // new alg. is starting
    BOOST_REQUIRE_EQUAL(reward_visitor.get_last_reward(), asset(new_reward_to_migrate, SP_SYMBOL));
}

BOOST_AUTO_TEST_SUITE_END()
}
