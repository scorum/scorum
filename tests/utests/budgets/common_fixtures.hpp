#pragma once

#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/database/budget_management_algorithms.hpp>

#include "defines.hpp"

#include "actor.hpp"

#include "service_wrappers.hpp"

namespace common_fixtures {

using namespace service_wrappers;

struct fixture : public shared_memory_fixture
{
    MockRepository mocks;

    fc::time_point_sec head_block_time = fc::time_point_sec::from_iso_string("2018-07-01T00:00:00");

    dynamic_global_property_service_wrapper dgp_service_fixture;

    fixture()
        : dgp_service_fixture(*this, mocks, [&](dynamic_global_property_object& p) {
            p.time = head_block_time;
            p.head_block_number = 1;
        })
    {
    }
};

struct budget_fixture : public fixture
{
    fc::time_point_sec start_time = head_block_time + fc::seconds(SCORUM_BLOCK_INTERVAL * 100);
    fc::time_point_sec deadline = start_time + fc::seconds(SCORUM_BLOCK_INTERVAL * 100);
    share_type balance = 1000200;
};

struct account_budget_fixture : public budget_fixture
{
    Actor alice;

    post_budget_service_wrapper post_budget_service_fixture;
    banner_budget_service_wrapper banner_budget_service_fixture;
    account_service_wrapper account_service_fixture;

    account_budget_fixture()
        : alice("alice")
        , post_budget_service_fixture(*this, mocks)
        , banner_budget_service_fixture(*this, mocks)
        , account_service_fixture(*this, mocks)
    {
        alice.scorum(asset(balance * 10, SCORUM_SYMBOL));
        account_service_fixture.add_actor(alice);
    }
};
}
