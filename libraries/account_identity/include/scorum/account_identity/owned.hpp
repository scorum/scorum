#pragma once

#include <fc/container/flat.hpp>
#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/transaction.hpp>

namespace scorum {
namespace account_identity {

void operation_get_owned_accounts(const scorum::protocol::operation& op,
                                  fc::flat_set<scorum::protocol::account_name_type>& result);

void transaction_get_owned_accounts(const scorum::protocol::transaction& tx,
                                    fc::flat_set<scorum::protocol::account_name_type>& result);
}
}
