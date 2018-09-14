#pragma once

#include "service_wrappers.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/betting/betting_math.hpp>
#include <scorum/protocol/betting/wincase.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/betting/betting_matcher.hpp>

namespace betting_common {

using namespace scorum::chain;
using namespace scorum::protocol;

using namespace service_wrappers;

struct betting_service_fixture_impl : public shared_memory_fixture
{
    const account_name_type moderator = "smit";

protected:
    MockRepository mocks;

    data_service_factory_i* dbs_services = mocks.Mock<data_service_factory_i>();
    betting_service_i* betting_svc = mocks.Mock<betting_service_i>();
    database_virtual_operations_emmiter_i* virt_op_emitter = mocks.Mock<database_virtual_operations_emmiter_i>();

    betting_service_fixture_impl()
        : betting_property(*this, mocks, [&](betting_property_object& bp) { bp.moderator = moderator; })
        , account_service(*this, mocks)
        , games(*this, mocks)
        , pending_bets(*this, mocks)
        , matched_bets(*this, mocks)
        , dgp_service(*this, mocks, [&](dynamic_global_property_object& p) {
            p.time = fc::time_point_sec::from_iso_string("2018-07-01T00:00:00");
            p.head_block_number = 1;
        })
    {
        mocks.OnCall(dbs_services, data_service_factory_i::betting_property_service)
            .ReturnByRef(betting_property.service());
        mocks.OnCall(dbs_services, data_service_factory_i::account_service).ReturnByRef(account_service.service());
        mocks.OnCall(dbs_services, data_service_factory_i::game_service).ReturnByRef(games.service());
        mocks.OnCall(dbs_services, data_service_factory_i::pending_bet_service).ReturnByRef(pending_bets.service());
        mocks.OnCall(dbs_services, data_service_factory_i::matched_bet_service).ReturnByRef(matched_bets.service());
        mocks.OnCall(dbs_services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(dgp_service.service());
        mocks.OnCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);
    }

    const account_name_type test_bet_better = "alice";
    const game_id_type test_bet_game = 15;
    const wincase_type test_bet_wincase = goal_home::yes();
    const std::string test_bet_k = "100/1";
    const asset test_bet_stake = ASSET_SCR(1e+9);

    const pending_bet_object& create_bet()
    {
        return create_bet(test_bet_better, test_bet_game, test_bet_wincase, test_bet_k, test_bet_stake);
    }

    const pending_bet_object& create_bet(const account_name_type& better,
                                         const game_id_type game,
                                         const wincase_type& wincase,
                                         const std::string& odds_value,
                                         const asset& stake)
    {
        return pending_bets.create([&](pending_bet_object& obj) {
            obj.created = dgp_service.service().head_block_time();
            obj.better = better;
            obj.game = game;
            obj.wincase = wincase;
            obj.odds_value = odds::from_string(odds_value);
            obj.stake = stake;
            obj.market = create_market(wincase);
        });
    }

public:
    service_base_wrapper<betting_property_service_i> betting_property;
    account_service_wrapper account_service;
    game_service_wrapper games;
    pending_bet_service_wrapper pending_bets;
    matched_service_wrapper matched_bets;
    dynamic_global_property_service_wrapper dgp_service;
};

struct betting_evaluator_fixture_impl : public betting_service_fixture_impl
{
protected:
    betting_evaluator_fixture_impl()
    {
    }

public:
    betting_service_i* betting_service_moc = mocks.Mock<betting_service_i>();
    betting_matcher_i* betting_matcher_moc = mocks.Mock<betting_matcher_i>();
};
}
