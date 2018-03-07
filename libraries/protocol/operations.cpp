#include <scorum/protocol/operations.hpp>

#include <scorum/protocol/operation_util_impl.hpp>

namespace scorum {
namespace protocol {

struct is_market_op_visitor
{
    typedef bool result_type;

    template <typename T> bool operator()(T&& v) const
    {
        return false;
    }
    bool operator()(const transfer_operation&) const
    {
        return true;
    }
    bool operator()(const transfer_to_scorumpower_operation&) const
    {
        return true;
    }
};

bool is_market_operation(const operation& op)
{
    return op.visit(is_market_op_visitor());
}

struct is_vop_visitor
{
    typedef bool result_type;

    template <typename T> bool operator()(const T& v) const
    {
        return v.is_virtual();
    }
};

bool is_virtual_operation(const operation& op)
{
    return op.visit(is_vop_visitor());
}
}
} // scorum::protocol

DEFINE_OPERATION_TYPE(scorum::protocol::operation)
