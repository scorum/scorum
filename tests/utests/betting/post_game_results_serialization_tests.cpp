#include <boost/test/unit_test.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/wincase_serialization.hpp>
#include <scorum/protocol/betting/game_serialization.hpp>

#include <defines.hpp>
#include <iostream>

namespace {
using namespace scorum;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

struct post_game_results_serialization_test_fixture
{
};

BOOST_FIXTURE_TEST_SUITE(game_serialization_tests, game_serialization_test_fixture)

SCORUM_TEST_CASE(post_game_results_json_serialization_test)
{
}

SCORUM_TEST_CASE(post_game_results_json_deserialization_test)
{
}

SCORUM_TEST_CASE(post_game_results_binary_serialization_test)
{
}

SCORUM_TEST_CASE(post_game_results_binary_deserialization_test)
{
}

BOOST_AUTO_TEST_SUITE_END()
}
