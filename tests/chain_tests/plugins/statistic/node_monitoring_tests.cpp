#include <boost/test/unit_test.hpp>

#include <scorum/app/api_context.hpp>

#include <scorum/blockchain_statistics/blockchain_statistics_plugin.hpp>
#include <scorum/blockchain_statistics/blockchain_statistics_api.hpp>
#include <scorum/blockchain_statistics/node_monitoring_api.hpp>

#include <scorum/protocol/block.hpp>

#include <chrono>
#include <thread>

#include "database_trx_integration.hpp"

using namespace scorum;
using namespace scorum::app;
using namespace scorum::protocol;

namespace blockchain_statistics_tests {

struct monitoring_database_fixture : public database_fixture::database_integration_fixture
{
    std::shared_ptr<scorum::blockchain_statistics::blockchain_statistics_plugin> _plugin;

    monitoring_database_fixture()
        : _api_ctx(app, API_NODE_MONITORING, std::make_shared<api_session_data>())
        , _api_call(_api_ctx)
    {
        boost::program_options::variables_map options;

        _plugin = app.register_plugin<scorum::blockchain_statistics::blockchain_statistics_plugin>();
        app.enable_plugin(_plugin->plugin_name());
        _plugin->plugin_initialize(options);
        _plugin->plugin_startup();

        open_database();
    }

    api_context _api_ctx;
    blockchain_statistics::node_monitoring_api _api_call;
};

} // namespace blockchain_statistics_tests

BOOST_FIXTURE_TEST_SUITE(node_monitoring_tests, blockchain_statistics_tests::monitoring_database_fixture)

class test_delay
{
    void sleep()
    {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

public:
    test_delay(chain::database& db)
    {
        db.pre_applied_block.connect([&](const signed_block& b) { this->sleep(); });
    }
};

SCORUM_TEST_CASE(check_block_processing_duration)
{
    BOOST_REQUIRE_EQUAL(_api_call.get_last_block_duration_microseconds(), 0u);

    test_delay delay(db);

    generate_block();

    BOOST_REQUIRE_GE(_api_call.get_last_block_duration_microseconds(), 10u);
}

SCORUM_TEST_CASE(check_total_shared_memory)
{
    const std::size_t total_avaliable_shmem = TEST_SHARED_MEM_SIZE_10MB / (1024 * 1024);

    BOOST_REQUIRE_EQUAL(_api_call.get_total_shared_memory_mb(), total_avaliable_shmem);

    generate_block();

    BOOST_REQUIRE_EQUAL(_api_call.get_total_shared_memory_mb(), total_avaliable_shmem);
}

SCORUM_TEST_CASE(check_free_shared_memory)
{
    BOOST_REQUIRE_LT(_api_call.get_free_shared_memory_mb(), _api_call.get_total_shared_memory_mb());
    BOOST_REQUIRE_GT(_api_call.get_free_shared_memory_mb(), 0u);

    generate_block();

    BOOST_REQUIRE_LT(_api_call.get_free_shared_memory_mb(), _api_call.get_total_shared_memory_mb());
    BOOST_REQUIRE_GT(_api_call.get_free_shared_memory_mb(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
