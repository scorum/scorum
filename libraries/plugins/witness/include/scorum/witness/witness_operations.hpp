#pragma once

#include <scorum/protocol/base.hpp>
#include <scorum/protocol/operation_util.hpp>

#include <scorum/app/plugin.hpp>

namespace scorum {
namespace witness {

using scorum::protocol::base_operation;
using scorum::chain::database;

class witness_plugin;

struct enable_content_editing_operation : base_operation
{
    protocol::account_name_type account;
    fc::time_point_sec relock_time;

    void validate() const;

    void get_required_active_authorities(flat_set<protocol::account_name_type>& a) const
    {
        a.insert(account);
    }
};

typedef fc::static_variant<enable_content_editing_operation> witness_plugin_operation;

DEFINE_PLUGIN_EVALUATOR(witness_plugin, witness_plugin_operation, enable_content_editing);
}
} // scorum::witness

FC_REFLECT(scorum::witness::enable_content_editing_operation, (account)(relock_time))

FC_REFLECT_TYPENAME(scorum::witness::witness_plugin_operation)

DECLARE_OPERATION_TYPE(scorum::witness::witness_plugin_operation)
