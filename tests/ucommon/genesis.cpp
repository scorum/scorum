#include "genesis.hpp"

#include <fc/time.hpp>

#include "defines.hpp"

genesis_state_type Genesis::generate()
{
    genesis_state.initial_timestamp = fc::time_point_sec(TEST_GENESIS_TIMESTAMP);
    genesis_state.initial_chain_id = fc::sha256::hash("tests");
    return genesis_state;
}
