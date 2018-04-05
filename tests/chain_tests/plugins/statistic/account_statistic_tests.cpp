#include <boost/test/unit_test.hpp>

#include <scorum/account_statistics/account_statistics_plugin.hpp>
#include <scorum/common_statistics/base_plugin_impl.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/schema/account_objects.hpp>

#include "database_trx_integration.hpp"

using namespace scorum;
using namespace scorum::account_statistics;
using namespace database_fixture;

namespace account_stat {

struct stat_database_fixture : public database_trx_integration_fixture
{
    stat_database_fixture()
    {
        init_plugin<scorum::account_statistics::account_statistics_plugin>();

        open_database();
        generate_block();
        validate_database();
    }

    const bucket_object& get_lifetime_bucket() const
    {
        const auto& bucket_idx = db.get_index<bucket_index>().indices().get<common_statistics::by_bucket>();
        auto itr = bucket_idx.find(boost::make_tuple(LIFE_TIME_PERIOD, fc::time_point_sec()));
        FC_ASSERT(itr != bucket_idx.end());
        return *itr;
    }
};
} // namespace account_stat

BOOST_FIXTURE_TEST_SUITE(account_statistic_tests, account_stat::stat_database_fixture)

SCORUM_TEST_CASE(account_transfers_stat_test)
{
    const char* buratino = "buratino";
    const char* maugli = "maugli";

    const auto& stat_map = get_lifetime_bucket().account_statistic;

    account_create(buratino, initdelegate.public_key);
    account_create(maugli, initdelegate.public_key);

    BOOST_REQUIRE(stat_map.find(buratino) == stat_map.end());
    BOOST_REQUIRE(stat_map.find(maugli) == stat_map.end());

    fund(buratino, SCORUM_MIN_PRODUCER_REWARD);

    BOOST_REQUIRE(stat_map.find(buratino) != stat_map.end());
    BOOST_REQUIRE(stat_map.find(maugli) == stat_map.end());

    {
        const auto& buratino_stat = stat_map.find(buratino)->second;

        BOOST_REQUIRE_EQUAL(buratino_stat.transfers_to, 1u);
        BOOST_REQUIRE_EQUAL(buratino_stat.scorum_received, SCORUM_MIN_PRODUCER_REWARD);
    }

    transfer_operation op;
    op.from = buratino;
    op.to = maugli;
    op.amount = SCORUM_MIN_PRODUCER_REWARD / 2;

    push_operation(op);

    BOOST_REQUIRE(stat_map.find(maugli) != stat_map.end());

    {
        const auto& buratino_acc = db.obtain_service<chain::dbs_account>().get_account(buratino);
        const auto& buratino_stat = stat_map.find(buratino)->second;

        BOOST_REQUIRE_EQUAL(buratino_stat.transfers_from, 1u);
        BOOST_REQUIRE_EQUAL(buratino_stat.scorum_sent, SCORUM_MIN_PRODUCER_REWARD / 2);
        BOOST_REQUIRE_EQUAL(buratino_stat.scorum_received - buratino_stat.scorum_sent, buratino_acc.balance);

        const auto& maugli_acc = db.obtain_service<chain::dbs_account>().get_account(maugli);
        const auto& maugli_stat = stat_map.find(maugli)->second;

        BOOST_REQUIRE_EQUAL(maugli_stat.transfers_to, 1u);
        BOOST_REQUIRE_EQUAL(maugli_stat.scorum_received, SCORUM_MIN_PRODUCER_REWARD / 2);
        BOOST_REQUIRE_EQUAL(maugli_stat.scorum_received, maugli_acc.balance);
    }
}

BOOST_AUTO_TEST_SUITE_END()
