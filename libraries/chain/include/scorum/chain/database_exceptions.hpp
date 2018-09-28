#pragma once

#include <scorum/protocol/exceptions.hpp>

#define SCORUM_DECLARE_OP_BASE_EXCEPTIONS(op_name)                                                                     \
    FC_DECLARE_DERIVED_EXCEPTION(op_name##_validate_exception, scorum::chain::operation_validate_exception,            \
                                 4040000 + 100 * protocol::operation::tag<protocol::op_name##_operation>::value,       \
                                 #op_name "_operation validation exception")                                           \
    FC_DECLARE_DERIVED_EXCEPTION(op_name##_evaluate_exception, scorum::chain::operation_evaluate_exception,            \
                                 4050000 + 100 * protocol::operation::tag<protocol::op_name##_operation>::value,       \
                                 #op_name "_operation evaluation exception")

#define SCORUM_DECLARE_OP_VALIDATE_EXCEPTION(exc_name, op_name, seqnum, msg)                                           \
    FC_DECLARE_DERIVED_EXCEPTION(                                                                                      \
        op_name##_##exc_name, scorum::chain::op_name##_validate_exception,                                             \
        4040000 + 100 * protocol::operation::tag<protocol::op_name##_operation>::value + seqnum, msg)

#define SCORUM_DECLARE_OP_EVALUATE_EXCEPTION(exc_name, op_name, seqnum, msg)                                           \
    FC_DECLARE_DERIVED_EXCEPTION(                                                                                      \
        op_name##_##exc_name, scorum::chain::op_name##_evaluate_exception,                                             \
        4050000 + 100 * protocol::operation::tag<protocol::op_name##_operation>::value + seqnum, msg)

#define SCORUM_DECLARE_INTERNAL_EXCEPTION(exc_name, seqnum, msg)                                                       \
    FC_DECLARE_DERIVED_EXCEPTION(internal_##exc_name, scorum::chain::internal_exception, 4990000 + seqnum, msg)

#ifdef DEBUG_PLUGINS
#define SCORUM_TRY_NOTIFY(signal, ...)                                                                                 \
    try                                                                                                                \
    {                                                                                                                  \
        signal(__VA_ARGS__);                                                                                           \
    }                                                                                                                  \
    FC_LOG_AND_RETHROW();
#else
#define SCORUM_TRY_NOTIFY(signal, ...)                                                                                 \
    try                                                                                                                \
    {                                                                                                                  \
        signal(__VA_ARGS__);                                                                                           \
    }                                                                                                                  \
    catch (const scorum::chain::plugin_exception& e)                                                                   \
    {                                                                                                                  \
        throw;                                                                                                         \
    }                                                                                                                  \
    catch (const fc::exception& e)                                                                                     \
    {                                                                                                                  \
        elog("Caught exception in plugin: ${e}", ("e", e.to_detail_string()));                                         \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        wlog("Caught unexpected exception in plugin");                                                                 \
    }
#endif

namespace scorum {
namespace chain {

FC_DECLARE_EXCEPTION(chain_exception, 4000000, "blockchain exception")
FC_DECLARE_DERIVED_EXCEPTION(database_query_exception,
                             scorum::chain::chain_exception,
                             4010000,
                             "database query exception")
FC_DECLARE_DERIVED_EXCEPTION(block_validate_exception,
                             scorum::chain::chain_exception,
                             4020000,
                             "block validation exception")
FC_DECLARE_DERIVED_EXCEPTION(transaction_exception,
                             scorum::chain::chain_exception,
                             4030000,
                             "transaction validation exception")
FC_DECLARE_DERIVED_EXCEPTION(operation_validate_exception,
                             scorum::chain::chain_exception,
                             4040000,
                             "operation validation exception")
FC_DECLARE_DERIVED_EXCEPTION(operation_evaluate_exception,
                             scorum::chain::chain_exception,
                             4050000,
                             "operation evaluation exception")
FC_DECLARE_DERIVED_EXCEPTION(utility_exception, scorum::chain::chain_exception, 4060000, "utility method exception")
FC_DECLARE_DERIVED_EXCEPTION(undo_database_exception,
                             scorum::chain::chain_exception,
                             4070000,
                             "undo database exception")
FC_DECLARE_DERIVED_EXCEPTION(unlinkable_block_exception, scorum::chain::chain_exception, 4080000, "unlinkable block")
FC_DECLARE_DERIVED_EXCEPTION(unknown_hardfork_exception,
                             scorum::chain::chain_exception,
                             4090000,
                             "chain attempted to apply unknown hardfork")
FC_DECLARE_DERIVED_EXCEPTION(plugin_exception, scorum::chain::chain_exception, 4100000, "plugin exception")
FC_DECLARE_DERIVED_EXCEPTION(block_log_exception, scorum::chain::chain_exception, 4110000, "block log exception")

FC_DECLARE_DERIVED_EXCEPTION(transaction_expiration_exception,
                             scorum::chain::transaction_exception,
                             4030100,
                             "transaction expiration exception")
FC_DECLARE_DERIVED_EXCEPTION(transaction_tapos_exception,
                             scorum::chain::transaction_exception,
                             4030200,
                             "transaction tapos exception")

FC_DECLARE_DERIVED_EXCEPTION(pop_empty_chain,
                             scorum::chain::undo_database_exception,
                             4070001,
                             "there are no blocks to pop")

SCORUM_DECLARE_OP_BASE_EXCEPTIONS(transfer);
//   SCORUM_DECLARE_OP_EVALUATE_EXCEPTION( from_account_not_whitelisted, transfer, 1, "owner mismatch" )

SCORUM_DECLARE_OP_BASE_EXCEPTIONS(account_create);
SCORUM_DECLARE_OP_EVALUATE_EXCEPTION(max_auth_exceeded, account_create, 1, "Exceeds max authority fan-out")
SCORUM_DECLARE_OP_EVALUATE_EXCEPTION(auth_account_not_found, account_create, 2, "Auth account not found")

SCORUM_DECLARE_OP_BASE_EXCEPTIONS(account_update);
SCORUM_DECLARE_OP_EVALUATE_EXCEPTION(max_auth_exceeded, account_update, 1, "Exceeds max authority fan-out")
SCORUM_DECLARE_OP_EVALUATE_EXCEPTION(auth_account_not_found, account_update, 2, "Auth account not found")

FC_DECLARE_DERIVED_EXCEPTION(internal_exception, scorum::chain::chain_exception, 4990000, "internal exception")

SCORUM_DECLARE_INTERNAL_EXCEPTION(verify_auth_max_auth_exceeded, 1, "Exceeds max authority fan-out")
SCORUM_DECLARE_INTERNAL_EXCEPTION(verify_auth_account_not_found, 2, "Auth account not found")
}
} // scorum::chain

#pragma once

#include <fc/exception/exception.hpp>
#include <scorum/protocol/exceptions.hpp>

namespace scorum {
namespace chain {
}
} // scorum::chain
