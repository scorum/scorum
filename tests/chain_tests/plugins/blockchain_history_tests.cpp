#include <boost/test/unit_test.hpp>

#include <scorum/blockchain_history/blockchain_history_plugin.hpp>
#include <scorum/blockchain_history/schema/account_history_object.hpp>
#include <scorum/blockchain_history/schema/applied_operation.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/schema/account_objects.hpp>

#include "database_trx_integration.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using fc::string;

using operation_map_type = std::map<uint32_t, blockchain_history::applied_operation>;

namespace account_stat {

struct history_database_fixture : public database_fixture::database_trx_integration_fixture
{
    std::shared_ptr<scorum::blockchain_history::blockchain_history_plugin> _plugin;

    history_database_fixture()
    {
        boost::program_options::variables_map options;

        _plugin = app.register_plugin<scorum::blockchain_history::blockchain_history_plugin>();
        _plugin->plugin_initialize(options);

        open_database();
        generate_block();
        validate_database();
    }

    template <typename history_object_type>
    operation_map_type get_operations_accomplished_by_account(const std::string& account_name)
    {
        const auto& idx = db.get_index<blockchain_history::history_index<history_object_type>>()
                              .indices()
                              .get<blockchain_history::by_account>();

        auto itr = idx.lower_bound(boost::make_tuple(account_name, uint32_t(-1)));
        if (itr != idx.end())
            idump((*itr));
        auto end = idx.upper_bound(boost::make_tuple(account_name, int64_t(0)));
        if (end != idx.end())
            idump((*end));

        operation_map_type result;
        while (itr != end)
        {
            result[itr->sequence] = db.get(itr->op);
            ++itr;
        }
        return std::move(result);
    }
};
} // namespace account_stat

BOOST_FIXTURE_TEST_SUITE(blockchain_history_tests, account_stat::history_database_fixture)

SCORUM_TEST_CASE(check_account_nontransfer_operation_only_in_full_history_test)
{
    const char* buratino = "buratino";

    account_create(buratino, initdelegate.public_key);

    operation_map_type buratino_full_ops
        = get_operations_accomplished_by_account<blockchain_history::account_history_object>(buratino);
    operation_map_type buratino_scr_ops
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
    operation_map_type buratino_sp_ops
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);

    BOOST_REQUIRE_EQUAL(buratino_full_ops.size(), 1u);
    BOOST_REQUIRE_EQUAL(buratino_scr_ops.size(), 0u);
    BOOST_REQUIRE_EQUAL(buratino_sp_ops.size(), 0u);

    BOOST_REQUIRE(buratino_full_ops[0].op.which() == operation::tag<account_create_with_delegation_operation>::value);
}

SCORUM_TEST_CASE(check_account_transfer_operation_in_full_and_transfers_to_scr_history_test)
{
    const char* buratino = "buratino";

    account_create(buratino, initdelegate.public_key);

    fund(buratino, SCORUM_MIN_PRODUCER_REWARD);

    operation_map_type buratino_full_ops
        = get_operations_accomplished_by_account<blockchain_history::account_history_object>(buratino);
    operation_map_type buratino_scr_ops
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
    operation_map_type buratino_sp_ops
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);

    BOOST_REQUIRE_EQUAL(buratino_full_ops.size(), 2u);
    BOOST_REQUIRE_EQUAL(buratino_scr_ops.size(), 1u);
    BOOST_REQUIRE_EQUAL(buratino_sp_ops.size(), 0u);

    BOOST_REQUIRE(buratino_full_ops[0].op.which() == operation::tag<account_create_with_delegation_operation>::value);
    BOOST_REQUIRE(buratino_full_ops[1].op.which() == operation::tag<transfer_operation>::value);
    BOOST_REQUIRE(buratino_scr_ops[0].op.which() == operation::tag<transfer_operation>::value);

    BOOST_REQUIRE(buratino_scr_ops[0].op == buratino_full_ops[1].op);
}

SCORUM_TEST_CASE(check_account_transfer_operation_in_full_and_transfers_to_sp_history_test)
{
    const char* buratino = "buratino";

    account_create(buratino, initdelegate.public_key);

    vest(buratino, SCORUM_MIN_PRODUCER_REWARD);

    operation_map_type buratino_full_ops
        = get_operations_accomplished_by_account<blockchain_history::account_history_object>(buratino);
    operation_map_type buratino_scr_ops
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
    operation_map_type buratino_sp_ops
        = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);

    BOOST_REQUIRE_EQUAL(buratino_full_ops.size(), 2u);
    BOOST_REQUIRE_EQUAL(buratino_scr_ops.size(), 0u);
    BOOST_REQUIRE_EQUAL(buratino_sp_ops.size(), 1u);

    BOOST_REQUIRE(buratino_full_ops[0].op.which() == operation::tag<account_create_with_delegation_operation>::value);
    BOOST_REQUIRE(buratino_full_ops[1].op.which() == operation::tag<transfer_to_scorumpower_operation>::value);
    BOOST_REQUIRE(buratino_sp_ops[0].op.which() == operation::tag<transfer_to_scorumpower_operation>::value);

    BOOST_REQUIRE(buratino_sp_ops[0].op == buratino_full_ops[1].op);
}

SCORUM_TEST_CASE(check_account_transfer_operation_history_test)
{
    const char* buratino = "buratino";
    const char* maugli = "maugli";

    account_create(buratino, initdelegate.public_key);
    account_create(maugli, initdelegate.public_key);

    fund(buratino, SCORUM_MIN_PRODUCER_REWARD);

    {
        operation_map_type buratino_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(maugli);

        BOOST_REQUIRE_EQUAL(buratino_ops.size(), 1u);
        BOOST_REQUIRE_EQUAL(maugli_ops.size(), 0u);

        transfer_operation& op = buratino_ops[0].op.get<transfer_operation>();
        BOOST_REQUIRE_EQUAL(op.from, TEST_INIT_DELEGATE_NAME);
        BOOST_REQUIRE_EQUAL(op.to, buratino);
        BOOST_REQUIRE_EQUAL(op.amount, SCORUM_MIN_PRODUCER_REWARD);
    }

    transfer_operation op;
    op.from = buratino;
    op.to = maugli;
    op.amount = SCORUM_MIN_PRODUCER_REWARD / 2;
    push_operation(op);

    {
        operation_map_type buratino_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_scr_history_object>(maugli);

        BOOST_REQUIRE_EQUAL(buratino_ops.size(), 2u);
        BOOST_REQUIRE_EQUAL(maugli_ops.size(), 1u);

        BOOST_REQUIRE(buratino_ops[1].op == op);
        BOOST_REQUIRE(maugli_ops[0].op == op);
    }
}

SCORUM_TEST_CASE(check_account_transfer_to_scorumpower_operation_history_test)
{
    const char* buratino = "buratino";
    const char* maugli = "maugli";

    account_create(buratino, initdelegate.public_key);
    account_create(maugli, initdelegate.public_key);

    fund(buratino, SCORUM_MIN_PRODUCER_REWARD);

    {
        operation_map_type buratino_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(maugli);

        BOOST_REQUIRE_EQUAL(buratino_ops.size(), 0u);
        BOOST_REQUIRE_EQUAL(maugli_ops.size(), 0u);
    }

    transfer_to_scorumpower_operation op;
    op.from = buratino;
    op.to = maugli;
    op.amount = SCORUM_MIN_PRODUCER_REWARD / 2;
    push_operation(op);

    {
        operation_map_type buratino_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(buratino);
        operation_map_type maugli_ops
            = get_operations_accomplished_by_account<blockchain_history::transfers_to_sp_history_object>(maugli);

        BOOST_REQUIRE_EQUAL(buratino_ops.size(), 1u);
        BOOST_REQUIRE_EQUAL(maugli_ops.size(), 1u);

        BOOST_REQUIRE(buratino_ops[0].op == op);
        BOOST_REQUIRE(maugli_ops[0].op == op);
    }
}

BOOST_AUTO_TEST_SUITE_END()
