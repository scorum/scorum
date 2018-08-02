#include <boost/test/unit_test.hpp>

#include "service_wrappers.hpp"

#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/services/betting_service.hpp>

namespace betting_service_tests {

using namespace scorum::chain;
using namespace service_wrappers;

class betting_service : public dbs_betting
{
public:
    betting_service(data_service_factory_i* pdbs_factory)
        : dbs_betting(*(database*)pdbs_factory)
    {
    }
};

struct betting_service_fixture_impl : public shared_memory_fixture
{
    const account_name_type moderator = "smit";

protected:
    MockRepository mocks;

    data_service_factory_i* dbs_services = mocks.Mock<data_service_factory_i>();

    betting_service_fixture_impl()
        : betting_property(*this, mocks, [&](betting_property_object& bp) { bp.moderator = moderator; })
        , bet(*this, mocks)
        , pending_bet(*this, mocks)
        , matched_bet(*this, mocks)
        , dgp_bet(*this, mocks, [&](dynamic_global_property_object& p) {
            p.time = fc::time_point_sec::from_iso_string("2018-07-01T00:00:00");
            p.head_block_number = 1;
        })
    {
        mocks.OnCall(dbs_services, data_service_factory_i::betting_property_service)
            .ReturnByRef(betting_property.service());
        mocks.OnCall(dbs_services, data_service_factory_i::bet_service).ReturnByRef(bet.service());
        mocks.OnCall(dbs_services, data_service_factory_i::pending_bet_service).ReturnByRef(pending_bet.service());
        mocks.OnCall(dbs_services, data_service_factory_i::matched_bet_service).ReturnByRef(matched_bet.service());
        mocks.OnCall(dbs_services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(dgp_bet.service());
    }

public:
    service_base_wrapper<betting_property_service_i> betting_property;
    bet_service_wrapper bet;
    pending_bet_service_wrapper pending_bet;
    matched_service_wrapper matched_bet;
    dynamic_global_property_service_wrapper dgp_bet;
};

struct betting_service_fixture : public betting_service_fixture_impl
{
    betting_service_fixture()
        : service(dbs_services)
    {
    }

    betting_service service;

    const account_name_type test_bet_better = "alice";
    const game_id_type test_bet_game = 15;
    const wincase_type test_bet_wincase = wincase11();
    const std::string test_bet_k = "100/1";
    const asset test_bet_stake = ASSET_SCR(1e+9);

    const bet_object& create_bet()
    {
        return service.create_bet(test_bet_better, test_bet_game, test_bet_wincase, test_bet_k, test_bet_stake);
    }

    const bet_object& create_bet(const account_name_type& better,
                                 const game_id_type game,
                                 const wincase_type& wincase,
                                 const std::string& odds_value,
                                 const asset& stake)
    {
        return service.create_bet(better, game, wincase, odds_value, stake);
    }

    template <typename... Args> void validate_invariants(Args... args)
    {
        std::array<bet_service_i::object_cref_type, sizeof...(args)> list = { args... };
        asset total_start = ASSET_NULL_SCR;
        for (const bet_service_i::object_type& b : list)
        {
            total_start += b.stake;
        }

        asset total_result = ASSET_NULL_SCR;
        for (const bet_service_i::object_type& b : list)
        {
            total_result += b.gain;
            total_result += b.rest_stake;
        }

        BOOST_REQUIRE_EQUAL(total_result, total_start);
    }
};

BOOST_FIXTURE_TEST_SUITE(betting_service_tests, betting_service_fixture)

SCORUM_TEST_CASE(is_betting_moderator_check)
{
    BOOST_CHECK(!service.is_betting_moderator("jack"));
    BOOST_CHECK(service.is_betting_moderator(moderator));
}

SCORUM_TEST_CASE(create_bet_check)
{
    const auto& new_bet = create_bet();

    BOOST_REQUIRE(bet.is_exists(new_bet.id));

    BOOST_CHECK_EQUAL(new_bet.better, test_bet_better);
    BOOST_CHECK_EQUAL(new_bet.game, test_bet_game);
    BOOST_CHECK_EQUAL(new_bet.value.to_string(), test_bet_k);
    BOOST_CHECK_EQUAL(new_bet.stake, test_bet_stake);
    BOOST_CHECK_EQUAL(new_bet.rest_stake, test_bet_stake);
    BOOST_CHECK_EQUAL(new_bet.potential_gain, test_bet_stake * new_bet.value - new_bet.stake);
    BOOST_CHECK_EQUAL(new_bet.gain, ASSET_NULL_SCR);
}

SCORUM_TEST_CASE(matching_not_found_and_created_pending_check)
{
    const auto& new_bet = create_bet();

    service.match(new_bet);

    BOOST_CHECK(pending_bet.is_exists());
    BOOST_CHECK(!matched_bet.is_exists());
}

SCORUM_TEST_CASE(matched_for_full_stake_check)
{
    const auto& bet1 = create_bet("alice", test_bet_game, wincase11(), "10/1", ASSET_SCR(1e+9));

    service.match(bet1); // call this one bacause every bet creation followed by 'match' in evaluators

    const auto& bet2 = create_bet("bob", test_bet_game, wincase12(), "10/9", ASSET_SCR(9e+9));

    service.match(bet2);

    BOOST_CHECK(!pending_bet.is_exists());
    BOOST_CHECK(matched_bet.is_exists());

    BOOST_CHECK_EQUAL(matched_bet.get().bet1._id, bet2.id._id);
    BOOST_CHECK_EQUAL(matched_bet.get().bet2._id, bet1.id._id);

    BOOST_CHECK_EQUAL(bet1.rest_stake, ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(bet2.rest_stake, ASSET_NULL_SCR);

    validate_invariants(bet1, bet2);
}

SCORUM_TEST_CASE(matched_for_part_stake_check)
{
    const auto& bet1 = create_bet("alice", test_bet_game, wincase11(), "10/1", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet1);

    // set not enough stake to pay gain 'alice'
    const auto& bet2 = create_bet("bob", test_bet_game, wincase12(), "10/9", ASSET_SCR(8e+9));

    service.match(bet2);

    BOOST_CHECK(pending_bet.is_exists());
    BOOST_CHECK(matched_bet.is_exists());

    BOOST_CHECK_EQUAL(matched_bet.get().bet1._id, bet2.id._id);
    BOOST_CHECK_EQUAL(matched_bet.get().bet2._id, bet1.id._id);

    BOOST_CHECK_EQUAL(bet1.rest_stake, ASSET_SCR(111'111'112));
    // <- 0.(1) SCR period give accuracy lag!
    BOOST_CHECK_EQUAL(bet2.rest_stake, ASSET_NULL_SCR);

    BOOST_CHECK_EQUAL(pending_bet.get().bet._id, bet1.id._id);

    validate_invariants(bet1, bet2);
}

SCORUM_TEST_CASE(matched_for_full_stake_with_more_than_one_matching_check)
{
    const auto& bet1 = create_bet("alice", test_bet_game, wincase11(), "10/1", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet1);

    const auto& bet2 = create_bet("bob", test_bet_game, wincase12(), "10/9", ASSET_SCR(8e+9)); // 8 SCR

    service.match(bet2);

    const auto& bet3 = create_bet("sam", test_bet_game, wincase12(), "10/9", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet3);
    // we have accuracy lag here (look match_found_for_part_stake_check) but
    //  bet2.stake + bet3.stake = bet1.gain
    // and bet1, bet2, bet3 should be matched

    BOOST_CHECK(!pending_bet.is_exists());
    BOOST_CHECK(matched_bet.is_exists());

    BOOST_CHECK_EQUAL(matched_bet.get(1).bet1._id, bet2.id._id);
    BOOST_CHECK_EQUAL(matched_bet.get(1).bet2._id, bet1.id._id);

    BOOST_CHECK_EQUAL(matched_bet.get(2).bet1._id, bet3.id._id);
    BOOST_CHECK_EQUAL(matched_bet.get(2).bet2._id, bet1.id._id);

    BOOST_CHECK_GT(bet1.gain, ASSET_NULL_SCR);
    BOOST_CHECK_GT(bet2.gain, ASSET_NULL_SCR);
    BOOST_CHECK_GT(bet3.gain, ASSET_NULL_SCR);

    validate_invariants(bet1, bet2, bet3);
}

SCORUM_TEST_CASE(matched_from_larger_potential_result_check)
{
    const auto& bet1 = create_bet("bob", test_bet_game, wincase12(), "10/9", ASSET_SCR(8e+9)); // 8 SCR

    service.match(bet1);

    const auto& bet2 = create_bet("sam", test_bet_game, wincase12(), "10/9", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet2);

    const auto& bet3 = create_bet("alice", test_bet_game, wincase11(), "10/1", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet3);

    BOOST_CHECK(!pending_bet.is_exists());
    BOOST_CHECK(matched_bet.is_exists());

    BOOST_CHECK_EQUAL(matched_bet.get(1).bet1._id, bet3.id._id);
    BOOST_CHECK_EQUAL(matched_bet.get(1).bet2._id, bet1.id._id);

    BOOST_CHECK_EQUAL(matched_bet.get(2).bet1._id, bet3.id._id);
    BOOST_CHECK_EQUAL(matched_bet.get(2).bet2._id, bet2.id._id);

    BOOST_CHECK_GT(bet1.gain, ASSET_NULL_SCR);
    BOOST_CHECK_GT(bet2.gain, ASSET_NULL_SCR);
    BOOST_CHECK_GT(bet3.gain, ASSET_NULL_SCR);

    validate_invariants(bet1, bet2, bet3);
}

SCORUM_TEST_CASE(matched_but_matched_value_vanishes_from_larger_check)
{
    const auto& bet1 = create_bet("bob", test_bet_game, wincase12(), "10000/9999", ASSET_SCR(8e+3)); // 0.000'008 SCR
    // it should produce minimal matched stake = 1e-9 SCR

    service.match(bet1);

    const auto& bet2 = create_bet("alice", test_bet_game, wincase11(), "10000/1", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet2);

    BOOST_CHECK(pending_bet.is_exists()); // 0.000'008 SCR can't be paid for huge 'alice' gain
    BOOST_CHECK(matched_bet.is_exists());

    BOOST_CHECK_EQUAL(matched_bet.get(1).bet1._id, bet2.id._id);
    BOOST_CHECK_EQUAL(matched_bet.get(1).bet2._id, bet1.id._id);

    BOOST_CHECK_GT(bet1.gain, ASSET_NULL_SCR);
    BOOST_CHECK_GT(bet2.gain, ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(bet2.gain, bet1.stake);

    validate_invariants(bet1, bet2);
}

SCORUM_TEST_CASE(matched_but_matched_value_vanishes_from_less_check)
{
    const auto& bet1 = create_bet("alice", test_bet_game, wincase11(), "10000/1", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet1);

    const auto& bet2 = create_bet("bob", test_bet_game, wincase12(), "10000/9999", ASSET_SCR(8e+3)); // 0.000'008 SCR
    // it should produce minimal matched stake = 1e-9 SCR

    service.match(bet2);

    BOOST_CHECK(pending_bet.is_exists()); // 0.000'008 SCR can't be paid for huge 'alice' gain
    BOOST_CHECK(matched_bet.is_exists());

    BOOST_CHECK_EQUAL(matched_bet.get(1).bet1._id, bet2.id._id);
    BOOST_CHECK_EQUAL(matched_bet.get(1).bet2._id, bet1.id._id);

    BOOST_CHECK_GT(bet1.gain, ASSET_NULL_SCR);
    BOOST_CHECK_GT(bet2.gain, ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(bet1.gain, bet2.stake);

    validate_invariants(bet1, bet2);
}

BOOST_AUTO_TEST_SUITE_END()
}
