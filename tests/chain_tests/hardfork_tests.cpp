#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include "database_trx_integration.hpp"
#include "actor.hpp"

#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

namespace hardfork_tests {

using namespace database_fixture;
using namespace scorum::chain;
using namespace scorum::protocol;

class db_node : public database_trx_integration_fixture
{
public:
    db_node()
        : hardfork_property_service(db.hardfork_property_service())
        , account_service(db.account_service())
        , witness_service(db.witness_service())
    {
    }

    hardfork_property_service_i& hardfork_property_service;
    account_service_i& account_service;
    witness_service_i& witness_service;

    uint32_t initialize()
    {
        open_database();

        set_witness(initdelegate);

        return produce_block();
    }

    void set_witness(const Actor& witness)
    {
        BOOST_REQUIRE(witness_service.is_exists(witness.name));
        _witness = witness.name;
        _witness_key = witness.private_key;
    }

    void set_lock_producing(bool lock)
    {
        _lock_producing = lock;
    }

    const Actor& witness() const
    {
        return _witness;
    }

    void push_block(const signed_block& new_block)
    {
        db.push_block(new_block, database::skip_nothing);
    }

    uint32_t produce_block()
    {
        fc::time_point_sec now = db.head_block_time();
        now += SCORUM_BLOCK_INTERVAL;
        auto slot = db.get_slot_at_time(now);
        BOOST_REQUIRE(_witness.name == db.get_scheduled_witness(slot));
        return produce_block_impl(db.get_slot_time(slot));
    }

    fc::time_point_sec try_produce_block(const fc::time_point_sec& last)
    {
        fc::time_point_sec now = last;
        now += SCORUM_BLOCK_INTERVAL;
        auto slot = db.get_slot_at_time(now);
        if (!_lock_producing && _witness.name == db.get_scheduled_witness(slot))
        {
            produce_block_impl(now);
            return fc::time_point_sec();
        }
        return now; // miss block
    }

    signed_block get_last_block()
    {
        return get_block(db.head_block_num());
    }

    signed_block get_block(uint32_t num)
    {
        fc::optional<signed_block> result = db.fetch_block_by_number(num);
        BOOST_REQUIRE(result.valid());
        return *result;
    }

    using transactions_type = std::vector<signed_transaction>;

    template <typename... Args> transactions_type create_accounts(Args... args)
    {
        std::array<Actor, sizeof...(args)> list = { args... };
        transactions_type ret;
        for (Actor& a : list)
        {
            ret.emplace_back(create_account(a));
        }
        return ret;
    }

    signed_transaction create_account(const Actor& account)
    {
        signed_transaction trx;

        account_create_with_delegation_operation op_create;
        op_create.new_account_name = account.name;
        op_create.creator = initdelegate.name;
        if (account.sp_amount.amount > SCORUM_MIN_PRODUCER_REWARD.amount)
            op_create.fee = ASSET_SCR(account.sp_amount.amount.value);
        else
            op_create.fee = SCORUM_MIN_PRODUCER_REWARD;
        op_create.delegation = asset(0, SP_SYMBOL);
        op_create.owner = authority(1, account.public_key, 1);
        op_create.active = authority(1, account.public_key, 1);
        op_create.posting = authority(1, account.post_key.get_public_key(), 1);
        op_create.memo_key = account.public_key;
        op_create.json_metadata = "{}";

        trx.operations.push_back(op_create);

        if (account.scr_amount.amount > 0)
        {
            transfer_operation op_transfer;
            op_transfer.from = initdelegate.name;
            op_transfer.to = account.name;
            op_transfer.amount = account.scr_amount;
            op_transfer.memo = "gift";

            trx.operations.push_back(op_transfer);
        }

        trx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(initdelegate.private_key, db.get_chain_id());
        trx.validate();
        db.push_transaction(trx, 0);
        return trx;
    }

    template <typename... Args> transactions_type create_witnesses(Args... args)
    {
        std::array<Actor, sizeof...(args)> list = { args... };
        transactions_type ret;
        for (Actor& a : list)
        {
            ret.emplace_back(create_witness(a));
        }
        return ret;
    }

    signed_transaction create_witness(const Actor& witness)
    {
        signed_transaction trx;

        witness_update_operation op_create;
        op_create.owner = witness.name;
        op_create.url = witness.name + ".info";
        op_create.block_signing_key = witness.private_key.get_public_key();

        trx.operations.push_back(op_create);

        account_witness_vote_operation op_vote;
        op_vote.account = witness.name;
        op_vote.witness = witness.name;
        op_vote.approve = true;

        trx.operations.push_back(op_vote);

        trx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(witness.private_key, db.get_chain_id());
        trx.validate();
        db.push_transaction(trx, default_skip);

        return trx;
    }

    void sync_with(db_node& other)
    {
        auto curr_bn = db.head_block_num();
        while (curr_bn < other.db.head_block_num())
        {
            ++curr_bn;
            push_block(other.get_block(curr_bn));
            BOOST_REQUIRE_EQUAL(db.head_block_num(), curr_bn);
            curr_bn = db.head_block_num();
        }
    }

    uint32_t current_hardfork() const
    {
        return _current_hardfork_id;
    }

protected:
    virtual void open_database_impl(const genesis_state_type& genesis)
    {
        genesis_state_type genesis_new_timestamp = genesis;
        genesis_new_timestamp.initial_timestamp = fc::time_point_sec(SCORUM_HARDFORK_0_1_TIME) + fc::days(365 * 5);
        database_integration_fixture::open_database_impl(genesis_new_timestamp);

        db.pre_apply_operation.connect([&](const operation_notification& note) { on_operation(note); });
    }

    void on_operation(const operation_notification& n)
    {
        n.op.weak_visit([&](const hardfork_operation& hf) { _current_hardfork_id = hf.hardfork_id; });
    }

    uint32_t produce_block_impl(const fc::time_point_sec& block_time)
    {
        uint32_t new_block_num = 0;
        try
        {
            auto b = db.generate_block(block_time, _witness.name, _witness_key, database::skip_nothing);
            wdump((b));
            new_block_num = b.block_num();
        }
        FC_CAPTURE_LOG_AND_RETHROW((_witness.name)(block_time))
        return new_block_num;
    }

    Actor _witness;
    private_key_type _witness_key;
    bool _lock_producing = false;
    uint32_t _current_hardfork_id = 0;
};

struct hardfork_fixture : public database_integration_fixture
{
    hardfork_fixture()
    {
    }

    template <typename... Nods> void broadcast_transaction(const signed_transaction& trx, Nods... nods)
    {
        std::array<std::reference_wrapper<db_node>, sizeof...(nods)> list = { nods... };
        for (db_node& node : list)
        {
            node.db.push_transaction(trx);
        }
    }

    template <typename... Nods> void broadcast_block(const signed_block& b, Nods... nods)
    {
        std::array<std::reference_wrapper<db_node>, sizeof...(nods)> list = { nods... };
        for (db_node& node : list)
        {
            node.push_block(b);
        }
    }

    void prepare_network()
    {
        SCORUM_MESSAGE("-- Start network: node_base, node_witness1, node_witness2, node_witness3");

        node_base.initialize();
        node_witness1.initialize();
        node_witness2.initialize();
        node_witness3.initialize();

        BOOST_CHECK_EQUAL(node_base.db.head_block_num(), 1u);
        BOOST_CHECK_EQUAL(node_witness1.db.head_block_num(), 1u);
        BOOST_CHECK_EQUAL(node_witness2.db.head_block_num(), 1u);
        BOOST_CHECK_EQUAL(node_witness3.db.head_block_num(), 1u);

        SCORUM_MESSAGE("-- Create accounts on node_base");

        node_base.create_accounts(witness1, witness2, witness3);

        BOOST_CHECK(node_base.account_service.is_exists(witness1.name));
        BOOST_CHECK(node_base.account_service.is_exists(witness2.name));
        BOOST_CHECK(node_base.account_service.is_exists(witness3.name));

        node_base.produce_block();

        SCORUM_MESSAGE("-- Create witnesses on node_base");

        node_base.create_witnesses(witness1, witness2, witness3);

        BOOST_CHECK(node_base.witness_service.is_exists(witness1.name));
        BOOST_CHECK(node_base.witness_service.is_exists(witness2.name));
        BOOST_CHECK(node_base.witness_service.is_exists(witness3.name));

        node_base.produce_block();

        SCORUM_MESSAGE("-- Synchronize node_witness1, node_witness2, node_witness3");

        node_witness1.sync_with(node_base);
        node_witness2.sync_with(node_base);
        node_witness3.sync_with(node_base);

        auto last_block_num = node_base.db.head_block_num();

        BOOST_CHECK_EQUAL(node_witness1.db.head_block_num(), last_block_num);
        BOOST_CHECK_EQUAL(node_witness2.db.head_block_num(), last_block_num);
        BOOST_CHECK_EQUAL(node_witness3.db.head_block_num(), last_block_num);

        BOOST_CHECK(node_witness1.account_service.is_exists(witness1.name));
        BOOST_CHECK(node_witness2.account_service.is_exists(witness2.name));
        BOOST_CHECK(node_witness3.account_service.is_exists(witness3.name));

        BOOST_CHECK(node_witness1.witness_service.is_exists(witness1.name));
        BOOST_CHECK(node_witness2.witness_service.is_exists(witness2.name));
        BOOST_CHECK(node_witness3.witness_service.is_exists(witness3.name));

        node_witness1.set_witness(witness1);
        node_witness2.set_witness(witness2);
        node_witness3.set_witness(witness3);

        SCORUM_MESSAGE("-- Wait next witness schedule updating");

        auto ci = 0;
        while (ci++ < SCORUM_MAX_WITNESSES - (decltype(ci))last_block_num)
        {
            node_base.produce_block();

            broadcast_block(node_base.get_last_block(), std::ref(node_witness1), std::ref(node_witness2),
                            std::ref(node_witness3));
        }

        last_block_num = node_base.db.head_block_num();

        BOOST_CHECK_EQUAL(node_witness1.db.head_block_num(), last_block_num);
        BOOST_CHECK_EQUAL(node_witness2.db.head_block_num(), last_block_num);
        BOOST_CHECK_EQUAL(node_witness3.db.head_block_num(), last_block_num);
    }

    void network_produce_block()
    {
        auto now = node_base.db.head_block_time();
        while (true)
        {
            auto next = node_base.try_produce_block(now);
            if (fc::time_point_sec() == next)
            {
                broadcast_block(node_base.get_last_block(), std::ref(node_witness1), std::ref(node_witness2),
                                std::ref(node_witness3));
                return;
            }
            next = node_witness1.try_produce_block(now);
            if (fc::time_point_sec() == next)
            {
                broadcast_block(node_witness1.get_last_block(), std::ref(node_base), std::ref(node_witness2),
                                std::ref(node_witness3));
                return;
            }
            next = node_witness2.try_produce_block(now);
            if (fc::time_point_sec() == next)
            {
                broadcast_block(node_witness2.get_last_block(), std::ref(node_base), std::ref(node_witness1),
                                std::ref(node_witness3));
                return;
            }
            next = node_witness3.try_produce_block(now);
            if (fc::time_point_sec() == next)
            {
                broadcast_block(node_witness3.get_last_block(), std::ref(node_base), std::ref(node_witness1),
                                std::ref(node_witness2));
                return;
            }
            now = next;
        }
    }

    Actor witness1 = "witness1";
    Actor witness2 = "witness2";
    Actor witness3 = "witness3";

    db_node node_base; // witness initdelegate
    db_node node_witness1;
    db_node node_witness2;
    db_node node_witness3;
};

BOOST_FIXTURE_TEST_SUITE(hardfork_tests, hardfork_fixture)

SCORUM_TEST_CASE(hardfork_apply_check)
{
    prepare_network();

    wdump((node_base.db.get_genesis_time()));

    SCORUM_MESSAGE("-- Most witnesses have not produced blocks yet");

    const auto& pre_hpo = node_base.hardfork_property_service.get();

    wdump((pre_hpo.next_hardfork));
    wdump((pre_hpo.next_hardfork_time.to_iso_string()));
    wdump((pre_hpo.last_hardfork));

    BOOST_REQUIRE_EQUAL(pre_hpo.last_hardfork, 0);

    BOOST_REQUIRE_EQUAL(node_base.current_hardfork(), 0);
    BOOST_REQUIRE_EQUAL(node_witness1.current_hardfork(), 0);
    BOOST_REQUIRE_EQUAL(node_witness2.current_hardfork(), 0);
    BOOST_REQUIRE_EQUAL(node_witness3.current_hardfork(), 0);

    // next hardfork set by most not producing witnesses
    BOOST_CHECK(pre_hpo.next_hardfork == hardfork_version());
    BOOST_CHECK_EQUAL(pre_hpo.next_hardfork_time.to_iso_string(), node_base.db.get_genesis_time().to_iso_string());

    SCORUM_MESSAGE("-- New witnesses become produce blocks");

    auto ci = 0;
    while (ci++ < SCORUM_MAX_WITNESSES)
    {
        network_produce_block();
    }

    SCORUM_MESSAGE("-- Hardfork applied for network");

    const auto& hpo = node_base.hardfork_property_service.get();

    wdump((hpo.next_hardfork));
    wdump((hpo.next_hardfork_time.to_iso_string()));
    wdump((hpo.last_hardfork));

    BOOST_REQUIRE_EQUAL(hpo.last_hardfork, SCORUM_NUM_HARDFORKS);
    BOOST_REQUIRE_EQUAL(hpo.processed_hardforks.size(), SCORUM_NUM_HARDFORKS + 1);

    auto hardfork_ver = hpo.current_hardfork_version;
    auto hardfork_time_str = hpo.processed_hardforks[hpo.last_hardfork].to_iso_string();

    wdump((hardfork_ver));
    wdump((hardfork_time_str));

    BOOST_CHECK(hpo.next_hardfork == hardfork_ver);
    BOOST_CHECK_EQUAL(hpo.next_hardfork_time.to_iso_string(), hardfork_time_str);

    BOOST_REQUIRE_EQUAL(node_base.current_hardfork(), SCORUM_NUM_HARDFORKS);
    BOOST_REQUIRE_EQUAL(node_witness1.current_hardfork(), SCORUM_NUM_HARDFORKS);
    BOOST_REQUIRE_EQUAL(node_witness2.current_hardfork(), SCORUM_NUM_HARDFORKS);
    BOOST_REQUIRE_EQUAL(node_witness3.current_hardfork(), SCORUM_NUM_HARDFORKS);

    SCORUM_MESSAGE("-- Hardfork applied for witnesses");

    const witness_object& w1 = node_base.witness_service.get(witness1.name);
    wdump((w1));
    const witness_object& w2 = node_base.witness_service.get(witness2.name);
    wdump((w2));
    const witness_object& w3 = node_base.witness_service.get(witness3.name);
    wdump((w3));

    BOOST_CHECK(w1.hardfork_version_vote == hardfork_ver);
    BOOST_CHECK(w2.hardfork_version_vote == hardfork_ver);
    BOOST_CHECK(w3.hardfork_version_vote == hardfork_ver);

    BOOST_CHECK_EQUAL(w1.hardfork_time_vote.to_iso_string(), hardfork_time_str);
    BOOST_CHECK_EQUAL(w2.hardfork_time_vote.to_iso_string(), hardfork_time_str);
    BOOST_CHECK_EQUAL(w3.hardfork_time_vote.to_iso_string(), hardfork_time_str);
}

SCORUM_TEST_CASE(too_few_wirnesses_for_hardfork_appling_check)
{
    prepare_network();

    SCORUM_MESSAGE("-- Only witness1 produces blocks");

    node_witness2.set_lock_producing(true);
    node_witness3.set_lock_producing(true);

    auto ci = 0;
    while (ci++ < SCORUM_MAX_WITNESSES)
    {
        network_produce_block();
    }

    SCORUM_MESSAGE("-- Hardfork is not applied for network");

    const auto& hpo = node_base.hardfork_property_service.get();

    wdump((hpo.next_hardfork));
    wdump((hpo.next_hardfork_time.to_iso_string()));
    wdump((hpo.last_hardfork));

    BOOST_REQUIRE_NE(hpo.last_hardfork, SCORUM_NUM_HARDFORKS);
    BOOST_REQUIRE_LT(hpo.processed_hardforks.size(), SCORUM_NUM_HARDFORKS + 1);

    BOOST_REQUIRE_EQUAL(node_base.current_hardfork(), 0);
    BOOST_REQUIRE_EQUAL(node_witness1.current_hardfork(), 0);
    BOOST_REQUIRE_EQUAL(node_witness2.current_hardfork(), 0);
    BOOST_REQUIRE_EQUAL(node_witness3.current_hardfork(), 0);
}

SCORUM_TEST_CASE(hardfork_apply_without_part_of_witnesses_check)
{
    prepare_network();

    SCORUM_MESSAGE("-- Only witness1, witness2 produce blocks");

    node_witness3.set_lock_producing(true);

    auto ci = 0;
    while (ci++ < SCORUM_MAX_WITNESSES)
    {
        network_produce_block();
    }

    SCORUM_MESSAGE("-- Hardfork applied for network");

    const auto& hpo = node_base.hardfork_property_service.get();

    wdump((hpo.next_hardfork));
    wdump((hpo.next_hardfork_time.to_iso_string()));
    wdump((hpo.last_hardfork));

    BOOST_REQUIRE_EQUAL(hpo.last_hardfork, SCORUM_NUM_HARDFORKS);
    BOOST_REQUIRE_EQUAL(hpo.processed_hardforks.size(), SCORUM_NUM_HARDFORKS + 1);

    auto hardfork_ver = hpo.current_hardfork_version;
    auto hardfork_time_str = hpo.processed_hardforks[hpo.last_hardfork].to_iso_string();

    wdump((hardfork_ver));
    wdump((hardfork_time_str));

    BOOST_CHECK(hpo.next_hardfork == hardfork_ver);
    BOOST_CHECK_EQUAL(hpo.next_hardfork_time.to_iso_string(), hardfork_time_str);

    BOOST_REQUIRE_EQUAL(node_base.current_hardfork(), SCORUM_NUM_HARDFORKS);
    BOOST_REQUIRE_EQUAL(node_witness1.current_hardfork(), SCORUM_NUM_HARDFORKS);
    BOOST_REQUIRE_EQUAL(node_witness2.current_hardfork(), SCORUM_NUM_HARDFORKS);
    BOOST_REQUIRE_EQUAL(node_witness3.current_hardfork(), SCORUM_NUM_HARDFORKS);

    const witness_object& pre_w1 = node_base.witness_service.get(witness1.name);
    wdump((pre_w1));
    const witness_object& pre_w2 = node_base.witness_service.get(witness2.name);
    wdump((pre_w2));
    const witness_object& pre_w3 = node_base.witness_service.get(witness3.name);
    wdump((pre_w3));

    SCORUM_MESSAGE("-- All witnesses start producing blocks");

    node_witness3.set_lock_producing(false);

    ci = 0;
    while (ci++ < SCORUM_MAX_WITNESSES)
    {
        network_produce_block();
    }

    SCORUM_MESSAGE("-- Hardfork applied for witnesses");

    const witness_object& w1 = node_base.witness_service.get(witness1.name);
    wdump((w1));
    const witness_object& w2 = node_base.witness_service.get(witness2.name);
    wdump((w2));
    const witness_object& w3 = node_base.witness_service.get(witness3.name);
    wdump((w3));

    BOOST_CHECK(w1.hardfork_version_vote == hardfork_ver);
    BOOST_CHECK(w2.hardfork_version_vote == hardfork_ver);
    BOOST_CHECK(w3.hardfork_version_vote == hardfork_version());

    BOOST_CHECK_EQUAL(w1.hardfork_time_vote.to_iso_string(), hardfork_time_str);
    BOOST_CHECK_EQUAL(w2.hardfork_time_vote.to_iso_string(), hardfork_time_str);
    BOOST_CHECK_EQUAL(w3.hardfork_time_vote.to_iso_string(), node_base.db.get_genesis_time().to_iso_string());
}

BOOST_AUTO_TEST_SUITE_END()
}
