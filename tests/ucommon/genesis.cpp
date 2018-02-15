#include "genesis.hpp"

#include <fc/time.hpp>

#include "defines.hpp"

genesis_state_type Genesis::generate()
{
    genesis_state.initial_timestamp = fc::time_point_sec(TEST_GENESIS_TIMESTAMP);
    genesis_state.initial_chain_id = TEST_CHAIN_ID;
    return genesis_state;
}
