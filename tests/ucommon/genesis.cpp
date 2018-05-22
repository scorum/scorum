#include "genesis.hpp"

#include <fc/time.hpp>

#include "defines.hpp"

genesis_state_type Genesis::generate(const fc::time_point_sec& initial_timestamp)
{
    genesis_state.initial_timestamp = initial_timestamp;
    genesis_state.initial_chain_id = TEST_CHAIN_ID;
    return genesis_state;
}
