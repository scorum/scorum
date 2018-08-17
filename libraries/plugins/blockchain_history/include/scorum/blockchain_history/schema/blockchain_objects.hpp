#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//

#ifndef BLOCKCHAIN_HISTORY_SPACE_ID
#define BLOCKCHAIN_HISTORY_SPACE_ID 7
#endif

namespace scorum {
namespace blockchain_history {

using namespace scorum::chain;

enum blockchain_history_object_type
{
    operations_history = (BLOCKCHAIN_HISTORY_SPACE_ID << 8),
    account_all_operations_history,
    account_scr_to_scr_transfers_history,
    account_scr_to_sp_transfers_history,
    account_sp_to_scr_withdrawals_history,
    filtered_not_virt_operations_history,
    filtered_virt_operations_history,
    filtered_market_operations_history,
    devcommittee_all_operations_history,
    devcommittee_scr_to_scr_transfers_history,
    devcommittee_sp_to_scr_withdrawals_history,
};
}
}
