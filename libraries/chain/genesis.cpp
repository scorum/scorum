#include <scorum/chain/database.hpp>
#include <scorum/chain/genesis_state.hpp>
#include <scorum/chain/dbs_budget.hpp>
#include <scorum/chain/dbs_reward.hpp>

#include <scorum/chain/account_object.hpp>
#include <scorum/chain/block_summary_object.hpp>
#include <scorum/chain/chain_property_object.hpp>
#include <scorum/chain/scorum_objects.hpp>

#include <scorum/chain/pool/reward_pool.hpp>

#include <fc/io/json.hpp>

#define SCORUM_DEFAULT_INIT_PUBLIC_KEY "STM5omawYzkrPdcEEcFiwLdEu7a3znoJDSmerNgf96J2zaHZMTpWs"
#define SCORUM_DEFAULT_GENESIS_TIME fc::time_point_sec(1508331600);
#define SCORUM_DEFAULT_INIT_SUPPLY (1000000u)

namespace scorum {
namespace chain {
namespace utils {

using namespace scorum::protocol;

void generate_default_genesis_state(genesis_state_type& genesis)
{
    const sp::public_key_type init_public_key(SCORUM_DEFAULT_INIT_PUBLIC_KEY);

    genesis.init_supply = SCORUM_DEFAULT_INIT_SUPPLY;
    genesis.init_rewards_supply = SCORUM_REWARDS_INITIAL_SUPPLY;
    genesis.initial_timestamp = SCORUM_DEFAULT_GENESIS_TIME;

    genesis.accounts.push_back({ "initdelegate", "", init_public_key, genesis.init_supply, uint64_t(0) });

    genesis.witness_candidates.push_back({ "initdelegate", init_public_key });

    genesis.initial_chain_id = fc::sha256::hash(fc::json::to_string(genesis));
}

} // namespace utils

//////////////////////////////////////////////////////////////////////////
fc::time_point_sec database::get_genesis_time() const
{
    return _const_genesis_time;
}

void database::init_genesis(const genesis_state_type& genesis_state)
{
    try
    {
        FC_ASSERT(genesis_state.initial_timestamp != time_point_sec(), "Must initialize genesis timestamp.");
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

        init_genesis_accounts(genesis_state);
        init_genesis_witnesses(genesis_state);
        init_genesis_witness_schedule(genesis_state);
        init_genesis_global_property_object(genesis_state);
        init_genesis_rewards(genesis_state);

        // Nothing to do
        for (int i = 0; i < 0x10000; i++)
            create<block_summary_object>([&](block_summary_object&) {});

        create<hardfork_property_object>(
            [&](hardfork_property_object& hpo) { hpo.processed_hardforks.push_back(get_genesis_time()); });
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::init_genesis_accounts(const genesis_state_type& genesis_state)
{
    const std::vector<genesis_state_type::account_type>& accounts = genesis_state.accounts;

    for (auto& account : accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");

        create<account_object>([&](account_object& a) {
            a.name = account.name;
            a.memo_key = account.public_key;
            a.balance = asset(account.scr_amount, SCORUM_SYMBOL);
            a.json_metadata = "{created_at: 'GENESIS'}";
            a.recovery_account = account.recovery_account;
        });

        create<account_authority_object>([&](account_authority_object& auth) {
            auth.account = account.name;
            auth.owner.add_authority(account.public_key, 1);
            auth.owner.weight_threshold = 1;
            auth.active = auth.owner;
            auth.posting = auth.active;
        });
    }
}

void database::init_genesis_witnesses(const genesis_state_type& genesis_state)
{
    const std::vector<genesis_state_type::witness_type>& witnesses = genesis_state.witness_candidates;

    for (auto& witness : witnesses)
    {
        FC_ASSERT(!witness.owner_name.empty(), "Witness 'owner_name' should not be empty.");

        create<witness_object>([&](witness_object& w) {
            w.owner = witness.owner_name;
            w.signing_key = witness.block_signing_key;
            w.schedule = witness_object::top20;
            w.hardfork_time_vote = get_genesis_time();
        });
    }
}

void database::init_genesis_witness_schedule(const genesis_state_type& genesis_state)
{
    const std::vector<genesis_state_type::witness_type>& witness_candidates = genesis_state.witness_candidates;

    create<witness_schedule_object>([&](witness_schedule_object& wso) {
        for (size_t i = 0; i < wso.current_shuffled_witnesses.size() && i < witness_candidates.size(); ++i)
        {
            wso.current_shuffled_witnesses[i] = witness_candidates[i].owner_name;
        }
    });
}

void database::init_genesis_global_property_object(const genesis_state_type& genesis_state)
{
    create<dynamic_global_property_object>([&](dynamic_global_property_object& gpo) {
        gpo.time = get_genesis_time();
        gpo.recent_slots_filled = fc::uint128::max_value();
        gpo.participation_count = 128;
        gpo.current_supply = asset(genesis_state.init_supply, SCORUM_SYMBOL);
        gpo.maximum_block_size = SCORUM_MAX_BLOCK_SIZE;

        gpo.total_reward_fund_scorum = asset(0, SCORUM_SYMBOL);
        gpo.total_reward_shares2 = 0;
    });
}

void database::init_genesis_rewards(const genesis_state_type& genesis_state)
{
    const auto& gpo = get_dynamic_global_properties();

    auto post_rf = create<reward_fund_object>([&](reward_fund_object& rfo) {
        rfo.name = SCORUM_POST_REWARD_FUND_NAME;
        rfo.last_update = head_block_time();
        rfo.percent_curation_rewards = SCORUM_1_PERCENT * 25;
        rfo.percent_content_rewards = SCORUM_100_PERCENT;
        rfo.reward_balance = gpo.total_reward_fund_scorum;
        rfo.author_reward_curve = curve_id::linear;
        rfo.curation_reward_curve = curve_id::square_root;
    });

    // As a shortcut in payout processing, we use the id as an array index.
    // The IDs must be assigned this way. The assertion is a dummy check to ensure this happens.
    FC_ASSERT(post_rf.id._id == 0);

    // We share initial fund between raward_pool and fund budget
    dbs_reward& reward_service = obtain_service<dbs_reward>();
    dbs_budget& budget_service = obtain_service<dbs_budget>();

    asset initial_reward_pool_supply(genesis_state.init_rewards_supply.amount
                                         * SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS
                                         / SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS,
                                     genesis_state.init_rewards_supply.symbol);
    fc::time_point deadline = get_genesis_time() + fc::days(SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS);

    reward_service.create_pool(initial_reward_pool_supply);
    budget_service.create_fund_budget(genesis_state.init_rewards_supply - initial_reward_pool_supply, deadline);
}

} // namespace chain
} // namespace scorum
