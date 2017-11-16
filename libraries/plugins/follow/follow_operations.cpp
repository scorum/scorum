#include <scorum/follow/follow_operations.hpp>

#include <scorum/protocol/operation_util_impl.hpp>

namespace scorum {
namespace follow {

void follow_operation::validate() const
{
    FC_ASSERT(follower != following, "You cannot follow yourself");
}

void reblog_operation::validate() const
{
    FC_ASSERT(account != author, "You cannot reblog your own content");
}
}
} // scorum::follow

DEFINE_OPERATION_TYPE(scorum::follow::follow_plugin_operation)
