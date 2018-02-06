#include <scorum/chain/database.hpp>
#include <scorum/chain/genesis_state.hpp>
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

#include <scorum/chain/genesis.hpp>

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

        BOOST_ATTRIBUTE_UNUSED
        db_genesis genesis(*this, genesis_state);

        // Nothing to do
        for (int i = 0; i < 0x10000; i++)
            create<block_summary_object>([&](block_summary_object&) {});

        create<hardfork_property_object>(
            [&](hardfork_property_object& hpo) { hpo.processed_hardforks.push_back(get_genesis_time()); });
    }
    FC_CAPTURE_AND_RETHROW()
}

scorum::chain::db_genesis::db_genesis(scorum::chain::database& db, const genesis_state_type& genesis_state)
    : _db(db)
    , _genesis_state(genesis_state)
{
    init_global_property_object();
    init_accounts();
    init_founders();
    init_witnesses();
    init_witness_schedule();
    init_rewards();
    init_registration_objects();
}

void db_genesis::init_accounts()
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    asset accounts_supply = _genesis_state.accounts_supply;
    for (auto& account : _genesis_state.accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");

        accounts_supply -= account.scr_amount;
    }

    FC_ASSERT(accounts_supply.amount == (share_value_type)0, "'accounts_supply' must be sum of all accounts supply.");

    for (auto& account : _genesis_state.accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");

        account_service.create_initial_account(account.name, account.public_key, account.scr_amount,
                                               account.recovery_account, "{\"created_at\": \"GENESIS\"}");
    }
}

void db_genesis::init_founders()
{
    dbs_dynamic_global_property& prop_service = _db.obtain_service<dbs_dynamic_global_property>();
    const auto& cprops = prop_service.get();

    dbs_account& account_service = _db.obtain_service<dbs_account>();

    asset founders_supply = _genesis_state.founders_supply;
    uint16_t total_sp_percent = (uint16_t)0;
    for (auto& founder : _genesis_state.founders)
    {
        FC_ASSERT(!founder.name.empty(), "Founder 'name' should not be empty.");
        account_service.check_account_existence(founder.name);
        FC_ASSERT(founder.sp_percent >= (uint16_t)0 && founder.sp_percent <= (uint16_t)100,
                  "Founder 'sp_percent' should be in range [0, 100]. ${1} received.", ("1", founder.sp_percent));
        FC_ASSERT(founder.sp_percent == (uint16_t)0 || founders_supply.amount > (share_value_type)0,
                  "Empty 'founders_supply'.");

        total_sp_percent += founder.sp_percent;
        if (total_sp_percent >= (uint16_t)100)
            break;
    }

    FC_ASSERT(total_sp_percent == (uint16_t)100, "Total 'sp_percent' must be 100%.");

    asset founders_supply_rest = founders_supply;
    account_name_type pitiful;
    for (auto& founder : _genesis_state.founders)
    {
        if (founder.sp_percent == (uint16_t)0)
            continue;

        const auto& founder_obj = account_service.get_account(founder.name);
        asset sp_bonus(0, VESTS_SYMBOL);
        sp_bonus.amount *= founder.sp_percent * SCORUM_1_PERCENT;
        sp_bonus.amount /= SCORUM_100_PERCENT;
        if (sp_bonus.amount > (share_value_type)0)
        {
            account_service.increase_vesting_shares(founder_obj, sp_bonus);
            _db.modify(cprops, [&](dynamic_global_property_object& props) { props.total_vesting_shares += sp_bonus; });
            founders_supply_rest -= sp_bonus;
        }
        else if (founder.sp_percent > (uint16_t)0)
        {
            pitiful = founder_obj.name;
        }
    }

    if (founders_supply_rest.amount > (share_value_type)0)
    {
        if (pitiful != account_name_type())
        {
            const auto& founder_obj = account_service.get_account(pitiful);
            account_service.increase_vesting_shares(founder_obj, founders_supply_rest);
            _db.modify(cprops, [&](dynamic_global_property_object& props) {
                props.total_vesting_shares += founders_supply_rest;
            });
        }
    }
}

void db_genesis::init_witnesses()
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    for (auto& witness : _genesis_state.witness_candidates)
    {
        FC_ASSERT(!witness.name.empty(), "Witness 'name' should not be empty.");
        account_service.check_account_existence(witness.name);
        FC_ASSERT(witness.block_signing_key != public_key_type(), "Witness 'block_signing_key' should not be empty.");

        _db.create<witness_object>([&](witness_object& w) {
            w.owner = witness.name;
            w.signing_key = witness.block_signing_key;
            w.schedule = witness_object::top20;
            w.hardfork_time_vote = _db.get_genesis_time();
        });
    }
}

void db_genesis::init_witness_schedule()
{
    const std::vector<genesis_state_type::witness_type>& witness_candidates = _genesis_state.witness_candidates;

    _db.create<witness_schedule_object>([&](witness_schedule_object& wso) {
        for (size_t i = 0; i < wso.current_shuffled_witnesses.size() && i < witness_candidates.size(); ++i)
        {
            wso.current_shuffled_witnesses[i] = witness_candidates[i].name;
        }
    });
}

void db_genesis::init_global_property_object()
{
    _db.create<dynamic_global_property_object>([&](dynamic_global_property_object& gpo) {
        gpo.time = _db.get_genesis_time();
        gpo.recent_slots_filled = fc::uint128::max_value();
        gpo.participation_count = 128;
        asset founders_supply = _genesis_state.founders_supply;
        gpo.circulating_capital = _genesis_state.accounts_supply + asset(founders_supply.amount, SCORUM_SYMBOL);
        gpo.total_supply
            = gpo.circulating_capital + _genesis_state.init_rewards_supply + _genesis_state.registration_supply;
        gpo.median_chain_props.maximum_block_size = SCORUM_MAX_BLOCK_SIZE;
    });
}

void db_genesis::init_rewards()
{
    const auto& post_rf = _db.create<reward_fund_object>([&](reward_fund_object& rfo) {
        rfo.last_update = _db.head_block_time();
        rfo.reward_balance = asset(0, SCORUM_SYMBOL);
        rfo.author_reward_curve = curve_id::linear;
        rfo.curation_reward_curve = curve_id::square_root;
    });

    // As a shortcut in payout processing, we use the id as an array index.
    // The IDs must be assigned this way. The assertion is a dummy check to ensure this happens.
    FC_ASSERT(post_rf.id._id == 0);

    // We share initial fund between raward_pool and fund budget
    dbs_reward& reward_service = _db.obtain_service<dbs_reward>();
    dbs_budget& budget_service = _db.obtain_service<dbs_budget>();

    asset initial_reward_pool_supply(_genesis_state.init_rewards_supply.amount
                                         * SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS
                                         / SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS,
                                     _genesis_state.init_rewards_supply.symbol());
    fc::time_point deadline = _db.get_genesis_time() + fc::days(SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS);

    reward_service.create_pool(initial_reward_pool_supply);
    budget_service.create_fund_budget(_genesis_state.init_rewards_supply - initial_reward_pool_supply, deadline);
}

void db_genesis::init_registration_objects()
{
    dbs_registration_pool& registration_pool_service = _db.obtain_service<dbs_registration_pool>();
    dbs_registration_committee& registration_committee_service = _db.obtain_service<dbs_registration_committee>();

    // create sorted items list form genesis unordered data
    using schedule_item_type = registration_pool_object::schedule_item;
    using schedule_items_type = std::map<uint8_t, schedule_item_type>;
    schedule_items_type items;
    for (const auto& genesis_item : _genesis_state.registration_schedule)
    {
        items.insert(schedule_items_type::value_type(
            genesis_item.stage, schedule_item_type{ genesis_item.users, genesis_item.bonus_percent }));
    }

    registration_pool_service.create_pool(_genesis_state.registration_supply, _genesis_state.registration_bonus, items);

    using account_names_type = std::vector<account_name_type>;
    account_names_type committee;
    for (const auto& member : _genesis_state.registration_committee)
    {
        committee.emplace_back(member);
    }
    registration_committee_service.create_committee(committee);
}

} // namespace chain
} // namespace scorum
