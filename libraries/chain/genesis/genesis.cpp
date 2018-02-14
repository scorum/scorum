#include <scorum/chain/database.hpp>
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

#include <scorum/chain/genesis/initializators/accounts_initializator.hpp>
#include <scorum/chain/genesis/initializators/founders_initializator.hpp>
#include <scorum/chain/genesis/initializators/witnesses_initializator.hpp>
#include <scorum/chain/genesis/initializators/registration_initializator.hpp>
#include <scorum/chain/genesis/initializators/registration_bonus_initializator.hpp>
#include <scorum/chain/genesis/initializators/rewards_initializator.hpp>
#include <scorum/chain/genesis/initializators/witness_schedule_initializator.hpp>
#include <scorum/chain/genesis/initializators/global_property_initializator.hpp>
#include <scorum/chain/genesis/initializators/steemit_bounty_account_initializator.hpp>

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
    : genesis::initializators_registry(db, genesis_state)
    , _db(db)
    , _genesis_state(genesis_state)
{
    register_initializator(new genesis::accounts_initializator_impl);
    register_initializator(new genesis::founders_initializator_impl);
    register_initializator(new genesis::witnesses_initializator_impl);
    register_initializator(new genesis::registration_initializator_impl);
    register_initializator(new genesis::registration_bonus_initializator_impl);
    register_initializator(new genesis::global_property_initializator_impl);
    register_initializator(new genesis::witness_schedule_initializator_impl);
    register_initializator(new genesis::rewards_initializator_impl);
    register_initializator(new genesis::steemit_bounty_account_initializator_impl);

    init(genesis::accounts_initializator);
    init(genesis::founders_initializator);
    init(genesis::witnesses_initializator);
    init(genesis::registration_initializator);
    init(genesis::registration_bonus_initializator);
    init(genesis::global_property_initializator);
    init(genesis::witness_schedule_initializator);
    init(genesis::rewards_initializator);
    init(genesis::steemit_bounty_account_initializator);
}

} // namespace chain
} // namespace scorum
