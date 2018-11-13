#pragma once

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/scorum_virtual_operations.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::operation;

struct database_virtual_operations_emmiter_i
{
    virtual void push_virtual_operation(const operation& op) = 0;
    virtual ~database_virtual_operations_emmiter_i() = default;
};
}
}
