#include <scorum/chain/database/database.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/reward.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/block_summary_object.hpp>
#include <scorum/chain/schema/chain_property_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

#include <scorum/chain/schema/reward_pool_object.hpp>

#include <scorum/chain/genesis/genesis.hpp>

#include <scorum/chain/genesis/initializators/accounts_initializator.hpp>
#include <scorum/chain/genesis/initializators/founders_initializator.hpp>
#include <scorum/chain/genesis/initializators/global_property_initializator.hpp>
#include <scorum/chain/genesis/initializators/registration_bonus_initializator.hpp>
#include <scorum/chain/genesis/initializators/registration_initializator.hpp>
#include <scorum/chain/genesis/initializators/rewards_initializator.hpp>
#include <scorum/chain/genesis/initializators/steemit_bounty_account_initializator.hpp>
#include <scorum/chain/genesis/initializators/witness_schedule_initializator.hpp>
#include <scorum/chain/genesis/initializators/witnesses_initializator.hpp>
#include <scorum/chain/genesis/initializators/dev_pool_initializator.hpp>

#include <fc/io/json.hpp>

#include <vector>
#include <map>

namespace scorum {
namespace chain {

fc::time_point_sec database::get_genesis_time() const
{
    return _const_genesis_time;
}

void database::init_genesis(const genesis_state_type& genesis_state)
{
    try
    {
        FC_ASSERT(genesis_state.initial_timestamp != time_point_sec::min(), "Must initialize genesis timestamp.");
        FC_ASSERT(genesis_state.witness_candidates.size() > 0, "Cannot start a chain with zero witnesses.");

        struct auth_inhibitor
        {
            auth_inhibitor(database& db)
                : db(db)
                , old_flags(db.node_properties().skip_flags)
            {
                db.node_properties().skip_flags |= skip_authority_check;
            }

            ~auth_inhibitor()
            {
                db.node_properties().skip_flags = old_flags;
            }

        private:
            database& db;
            uint32_t old_flags;
        } inhibitor(*this);

        _const_genesis_time = genesis_state.initial_timestamp;
        create<chain_property_object>([&](chain_property_object& cp) { cp.chain_id = genesis_state.initial_chain_id; });

        BOOST_ATTRIBUTE_UNUSED
        db_genesis genesis(*this, genesis_state);

        // Nothing to do
        for (int i = 0; i <= SCORUM_BLOCKID_POOL_SIZE; ++i)
            create<block_summary_object>([&](block_summary_object&) {});

        create<hardfork_property_object>(
            [&](hardfork_property_object& hpo) { hpo.processed_hardforks.push_back(get_genesis_time()); });
    }
    FC_CAPTURE_AND_RETHROW()
}

db_genesis::db_genesis(scorum::chain::database& db, const genesis_state_type& genesis_state)
    : _db(db)
    , _genesis_state(genesis_state)
{
    genesis::accounts_initializator_impl accounts_initializator;
    genesis::founders_initializator_impl founders_initializator;
    genesis::global_property_initializator_impl global_property_initializator;
    genesis::registration_bonus_initializator_impl registration_bonus_initializator;
    genesis::registration_initializator_impl registration_initializator;
    genesis::rewards_initializator_impl rewards_initializator;
    genesis::steemit_bounty_account_initializator_impl steemit_bounty_account_initializator;
    genesis::witness_schedule_initializator_impl witness_schedule_initializator;
    genesis::witnesses_initializator_impl witnesses_initializator;
    genesis::dev_pool_initializator_impl dev_pool_initializator;

    genesis::initializator_context ctx(db, genesis_state);

    accounts_initializator.after(global_property_initializator).apply(ctx);
    rewards_initializator.after(global_property_initializator).apply(ctx);
    founders_initializator.after(accounts_initializator).apply(ctx);
    witnesses_initializator.after(accounts_initializator).apply(ctx);
    registration_initializator.after(accounts_initializator).apply(ctx);
    steemit_bounty_account_initializator.after(accounts_initializator).apply(ctx);
    registration_bonus_initializator.after(registration_initializator).after(accounts_initializator).apply(ctx);
    witness_schedule_initializator.after(witnesses_initializator).apply(ctx);
    dev_pool_initializator.after(global_property_initializator).apply(ctx);
}

} // namespace chain
} // namespace scorum
