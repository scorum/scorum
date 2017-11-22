#include <scorum/chain/genesis_state.hpp>
#include <fc/io/json.hpp>

#define SCORUM_DEFAULT_INIT_PUBLIC_KEY "SCR5omawYzkrPdcEEcFiwLdEu7a3znoJDSmerNgf96J2zaHZMTpWs"
#define SCORUM_DEFAULT_GENESIS_TIME fc::time_point_sec(1508331600);
#define SCORUM_DEFAULT_INIT_SUPPLY (1000000u)

namespace scorum {
namespace chain {
namespace utils {

void generate_default_genesis_state(genesis_state_type& genesis)
{
    const sp::public_key_type init_public_key(SCORUM_DEFAULT_INIT_PUBLIC_KEY);

    genesis.init_supply = SCORUM_DEFAULT_INIT_SUPPLY;
    genesis.initial_timestamp = SCORUM_DEFAULT_GENESIS_TIME;

    genesis.accounts.push_back({ "initdelegate", "", init_public_key, genesis.init_supply, uint64_t(0) });

    genesis.witness_candidates.push_back({ "initdelegate", init_public_key });

    genesis.initial_chain_id = fc::sha256::hash(fc::json::to_string(genesis));
}

} // namespace utils
} // namespace chain
} // namespace scorum
