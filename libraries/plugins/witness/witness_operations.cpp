#include <scorum/witness/witness_operations.hpp>

#include <scorum/protocol/operation_util_impl.hpp>

namespace scorum { namespace witness {

void enable_content_editing_operation::validate()const
{
   chain::validate_account_name( account );
}

} } // scorum::witness

DEFINE_OPERATION_TYPE( scorum::witness::witness_plugin_operation )
