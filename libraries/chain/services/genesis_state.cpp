#include <scorum/chain/services/genesis_state.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {

dbs_genesis_state::dbs_genesis_state(database& db)
    : _base_type(db)
{
}

const fc::time_point_sec& dbs_genesis_state::get_lock_withdraw_sp_until_timestamp() const
{
    return db_impl().genesis_state().lock_withdraw_sp_until_timestamp;
}
}
}
