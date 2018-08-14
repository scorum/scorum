#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include "database_trx_integration.hpp"
#include "actor.hpp"

#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/account_objects.hpp>

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

    uint32_t initialize(int hardfork = 0)
    {
        _hardfork = hardfork;

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
        uint32_t new_block_num = 0;
        auto slot = db.get_slot_at_time(now);
        if (_witness.name == db.get_scheduled_witness(slot))
        {
            try
            {
                auto b = db.generate_block(db.get_slot_time(slot), _witness.name, _witness_key, database::skip_nothing);
                new_block_num = b.block_num();
            }
            FC_CAPTURE_LOG_AND_RETHROW((_witness.name)(now))
        }
        return new_block_num;
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

protected:
    virtual void open_database_impl(const genesis_state_type& genesis)
    {
        database_integration_fixture::open_database_impl(genesis);

        db.applied_block.connect([&](const signed_block& b) { on_block(b); });
    }

    void on_block(const signed_block&)
    {
        if (_hardfork > 0 && !_hardfork_initialized)
        {
            db.set_hardfork(_hardfork);
            _hardfork_initialized = true;
        }
    }

    int _hardfork = 0;
    bool _hardfork_initialized = false;
    Actor _witness;
    private_key_type _witness_key;
};

struct hardfork_fixture
{
    hardfork_fixture()
    {
    }

    Actor wintess1 = "witness1";
    Actor wintess2 = "witness2";

    db_node node_base1;
    db_node node_base2;
    db_node node_wintess1;
    db_node node_wintess2;
};

BOOST_FIXTURE_TEST_SUITE(hardfork_tests, hardfork_fixture)

SCORUM_TEST_CASE(hardfork_empty_all_witnesses_has_same_hardfork_check)
{
    SCORUM_MESSAGE("-- Start initial node1 and node2");

    node_base1.initialize();
    node_base2.initialize();

    BOOST_CHECK_EQUAL(node_base1.db.head_block_num(), 1u);
    BOOST_CHECK_EQUAL(node_base2.db.head_block_num(), 1u);
    BOOST_CHECK(node_base1.hardfork_property_service.get().current_hardfork_version == hardfork_version());
    BOOST_CHECK(node_base2.hardfork_property_service.get().current_hardfork_version == hardfork_version());

    SCORUM_MESSAGE("-- Create account on node1");

    {
        auto txs = node_base1.create_accounts(wintess1, wintess2);

        BOOST_CHECK(node_base1.account_service.is_exists(wintess1.name));
        BOOST_CHECK(node_base1.account_service.is_exists(wintess2.name));

        for (const auto& tx : txs)
        {
            node_base2.db.push_transaction(tx);
        }
    }

    node_base1.produce_block();

    BOOST_CHECK_EQUAL(node_base1.db.head_block_num(), 2u);
    BOOST_CHECK_EQUAL(node_base2.db.head_block_num(), 1u);

    node_base2.push_block(node_base1.get_last_block());

    BOOST_CHECK_EQUAL(node_base2.db.head_block_num(), 2u);

    SCORUM_MESSAGE("-- Create witnesses on node2");

    {
        auto txs = node_base2.create_witnesses(wintess1, wintess2);

        BOOST_CHECK(node_base2.witness_service.is_exists(wintess1.name));
        BOOST_CHECK(node_base2.witness_service.is_exists(wintess2.name));

        for (const auto& tx : txs)
        {
            node_base1.db.push_transaction(tx);
        }
    }

    node_base2.produce_block();

    node_base1.push_block(node_base2.get_last_block());

    BOOST_CHECK(node_base1.witness_service.is_exists(wintess1.name));
    BOOST_CHECK(node_base1.witness_service.is_exists(wintess2.name));

    SCORUM_MESSAGE("-- Start initial node_witness1 and node_witness2");

    node_wintess1.initialize();
    node_wintess2.initialize();

    node_wintess1.sync_with(node_base1);
    node_wintess2.sync_with(node_base1);

    BOOST_CHECK(node_wintess1.witness_service.is_exists(wintess1.name));
    BOOST_CHECK(node_wintess2.witness_service.is_exists(wintess2.name));

    node_wintess1.set_witness();
}

SCORUM_TEST_CASE(hardfork_1_all_witnesses_has_same_hardfork_check)
{
    // TODO
}

SCORUM_TEST_CASE(hardfork_1_2_some_witnesses_has_different_hardfork_check)
{
    // TODO
}

BOOST_AUTO_TEST_SUITE_END()
}
