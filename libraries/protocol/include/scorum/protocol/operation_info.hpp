#pragma once
#include <scorum/protocol/operations.hpp>

namespace scorum {
namespace protocol {

struct operation_info
{
    operation_info(const operation& op)
        : _operation(op)
    {
    }

    operator std::string() const
    {
        return _operation.visit(serializing_visitor{});
    }

private:
    struct serializing_visitor
    {
        using result_type = std::string;
        template <typename T> std::string operator()(const T&)
        {
            return fc::get_typename<T>().name();
        }
    };

    const operation& _operation;
};
}
}