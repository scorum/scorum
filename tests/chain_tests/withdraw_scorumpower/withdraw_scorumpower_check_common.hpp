#pragma once

#include "database_trx_integration.hpp"
#include <scorum/blockchain_monitoring/blockchain_monitoring_plugin.hpp>
#include <scorum/blockchain_monitoring/blockchain_statistics_api.hpp>
#include <scorum/app/api_context.hpp>

namespace scorum {
namespace chain {

class dev_pool_service_i;
class account_service_i;
class withdraw_scorumpower_service_i;
class withdraw_scorumpower_route_service_i;
class dynamic_global_property_service_i;

} // namespace chain
} // namespace scorum

namespace database_fixture {

using namespace scorum::chain;
using namespace scorum::app;
using namespace scorum::blockchain_monitoring;

class withdraw_scorumpower_check_fixture : public database_trx_integration_fixture
{
public:
    withdraw_scorumpower_check_fixture();

    account_service_i& account_service;
    dev_pool_service_i& pool_service;
    withdraw_scorumpower_service_i& withdraw_scorumpower_service;
    withdraw_scorumpower_route_service_i& withdraw_scorumpower_route_service;
    dynamic_global_property_service_i& dynamic_global_property_service;

protected:
    void set_dev_pool_balance(const asset& sp_balance = ASSET_NULL_SP, const asset& scr_balance = ASSET_NULL_SCR);
};

} // namespace database_fixture
