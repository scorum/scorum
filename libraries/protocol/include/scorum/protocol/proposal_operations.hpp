#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/operation_util.hpp>
#include <fc/static_variant.hpp>

namespace scorum {
namespace protocol {

struct add_member_operation
{
    account_name_type account_name;
};

struct exclude_member_operation
{
    account_name_type account_name;
};

// clang-format off
using proposal_operation = fc::static_variant<add_member_operation,
                                              exclude_member_operation>;
// clang-format on

} // namespace protocol
} // namespace scorum

FC_REFLECT(scorum::protocol::add_member_operation, (account_name))
FC_REFLECT(scorum::protocol::exclude_member_operation, (account_name))

DECLARE_OPERATION_SERIALIZATOR(scorum::protocol::proposal_operation)
FC_REFLECT_TYPENAME(scorum::protocol::proposal_operation)
