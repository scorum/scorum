#include <boost/test/unit_test.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/chain/database/database.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/witness_schedule.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/blockchain_history/blockchain_history_plugin.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>
#include <scorum/chain/database_exceptions.hpp>

#include "database_default_integration.hpp"
#include <random>

const int witnesses_count = 43;

namespace {
using namespace database_fixture;
using namespace scorum::protocol;

class witness : public database_trx_integration_fixture
{
public:
    std::vector<Actor> witnesses;
    int witness_no;
    Actor account;

    witness(int witness_no)
        : witness_no(witness_no)
    {
        initdelegate.scorumpower(asset(TEST_ACCOUNTS_INITIAL_SUPPLY.amount, SP_SYMBOL));
        initdelegate.scorum(TEST_ACCOUNTS_INITIAL_SUPPLY);

        auto gen = Genesis::create()
                       .accounts_supply(TEST_ACCOUNTS_INITIAL_SUPPLY * witnesses_count)
                       .rewards_supply(TEST_REWARD_INITIAL_SUPPLY * witnesses_count)
                       .dev_committee(initdelegate)
                       .steemit_bounty_accounts_supply(
                           asset(TEST_ACCOUNTS_INITIAL_SUPPLY.amount * witnesses_count, SP_SYMBOL))
                       .steemit_bounty_accounts(initdelegate)
                       .witnesses(initdelegate)
                       .accounts(initdelegate);

        witnesses.push_back(initdelegate);

        for (int i = 1; i < witnesses_count; ++i)
        {
            Actor witness = "witness" + std::to_string(i + 1);
            witness.scorumpower(asset(TEST_ACCOUNTS_INITIAL_SUPPLY.amount, SP_SYMBOL));
            witness.scorum(TEST_ACCOUNTS_INITIAL_SUPPLY);
            // actor(initdelegate).give_sp(witness, 1e8);

            witnesses.push_back(witness);
            gen.accounts(witnesses.back());
            gen.witnesses(witnesses.back());
            gen.steemit_bounty_accounts(witnesses.back());
        }

        account = witnesses[witness_no];

        open_database(gen.generate());

        auto& wso = db.obtain_service<dbs_witness_schedule>().get();
        db.modify(wso, [](witness_schedule_object& o) { o.num_scheduled_witnesses = 21; });
    }

    virtual void open_database_impl(const genesis_state_type& genesis) override
    {
        database_integration_fixture::open_database_impl(genesis);
    }
};

class fork_db_fixture
{
public:
    fork_db_fixture()
        : distr(1, 42)
    {
        // std::default_random_engine

        for (int i = 0; i < witnesses_count; ++i)
            w.push_back(std::make_shared<witness>(i));
    }

    void vote(std::shared_ptr<witness> who, std::shared_ptr<witness> whom, std::shared_ptr<witness> sign_with)
    {
        account_witness_vote_operation op;
        op.account = who->account.name;
        op.witness = whom->account.name;
        op.approve = true;

        sign_with->push_operation(op, sign_with->account.private_key, false);
    }

    signed_block gen_block(std::shared_ptr<witness> wit, int slot_no)
    {
        return wit->db.generate_block(wit->db.get_slot_time(slot_no), wit->account.name, wit->account.private_key,
                                      database::skip_nothing);
    }

    void push(std::shared_ptr<witness> wit, signed_block& block)
    {
        wit->db.push_block(block, database::skip_nothing);
    }

    const fc::array<account_name_type, 21>& get_schedule(std::shared_ptr<witness> wit) const
    {
        auto& wso = wit->db.obtain_service<dbs_witness_schedule>().get();
        return wso.current_shuffled_witnesses;
    }

public:
    std::vector<std::shared_ptr<witness>> w;
    std::default_random_engine eng;
    std::uniform_int_distribution<> distr;
};

BOOST_FIXTURE_TEST_SUITE(fork_tests, fork_db_fixture)

BOOST_AUTO_TEST_CASE(should_throw_unlinkable_block_exception)
{
    auto& wso = w[0]->db.obtain_service<dbs_witness_schedule>().get();
    int i = 0;
    for (const auto& w : wso.current_shuffled_witnesses)
        BOOST_TEST_MESSAGE(++i << ": " << w);

    // starting from w[1] because we cannot select slot '0' directly.
    auto block0 = gen_block(w[1], 1);
    push(w[0], block0);
    push(w[2], block0);
    push(w[3], block0);
    push(w[4], block0);

    auto block1 = gen_block(w[2], 1);
    push(w[0], block1);
    push(w[1], block1);
    // push(w[3], block1);
    push(w[4], block1);

    auto block2 = gen_block(w[3], 2); // since w[3] skipped 'block1', 'block2' goes into fork db for each witness. since
    // we do not pushing block into main db, we do not increment dpo.current_aslot
    push(w[0], block2);
    push(w[1], block2);
    push(w[2], block2);
    push(w[4], block2);

    auto block3 = gen_block(w[4], 2);
    push(w[0], block3);
    push(w[1], block3);
    push(w[2], block3);

    BOOST_REQUIRE_THROW(push(w[3], block3), unlinkable_block_exception);
    push(w[3], block0); // syncing
    push(w[3], block1);
    push(w[3], block3);
}

BOOST_AUTO_TEST_CASE(circulating_capital_mismatch)
{
    auto& wso = w[0]->db.obtain_service<dbs_witness_schedule>().get();
    int i = 0;
    for (const auto& w : wso.current_shuffled_witnesses)
        BOOST_TEST_MESSAGE(++i << ": " << w);

    // starting from w[1] because we cannot select slot '0' directly.
    auto block0 = gen_block(w[1], 1);
    push(w[0], block0);
    push(w[2], block0);
    push(w[3], block0);
    push(w[4], block0);

    auto block1 = gen_block(w[2], 1);
    push(w[0], block1);
    push(w[1], block1);
    // push(w[3], block1);
    push(w[4], block1);

    auto block2 = gen_block(w[3], 2); // since w[3] skipped 'block1', 'block2' goes into fork db for each witness. since
    // we do not pushing block into main db, we do not increment dpo.current_aslot
    push(w[0], block2);
    push(w[1], block2);
    push(w[2], block2);
    push(w[4], block2);

    auto block3 = gen_block(w[4], 2);
    push(w[0], block3);
    push(w[1], block3);
    push(w[2], block3);

    BOOST_REQUIRE_THROW(push(w[3], block3), unlinkable_block_exception);
    push(w[3], block0); // syncing
    push(w[3], block1);
    push(w[3], block3);

    const auto& w3_account_service = w[3]->db.account_service();
    const auto& w2_acc_from_w3_db = w3_account_service.get_account(w[2]->account.name);

    const auto& w2_account_service = w[2]->db.account_service();
    const auto& w2_acc_from_w2_db = w2_account_service.get_account(w[2]->account.name);

    BOOST_REQUIRE_EQUAL(w2_acc_from_w2_db.scorumpower, w2_acc_from_w3_db.scorumpower);
}

BOOST_AUTO_TEST_SUITE_END()
}