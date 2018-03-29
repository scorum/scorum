#include <scorum/blockchain_history/schema/operation_objects.hpp>

namespace scorum {
namespace blockchain_history {

void update_filtered_operation_index(const operation_object& object,
                                     const operation& op,
                                     const filtered_operation_creator_type& create)
{
    create(applied_operation_type::all, object.id);
    if (is_virtual_operation(op))
    {
        create(applied_operation_type::virt, object.id);
    }
    else
    {
        create(applied_operation_type::not_virt, object.id);
    }
    if (is_market_operation(op))
    {
        create(applied_operation_type::market, object.id);
    }
}

bool operation_type_filter(const operation& op, const applied_operation_type& opt)
{
    switch (opt)
    {
    case applied_operation_type::not_virt:
        return !is_virtual_operation(op);
    case applied_operation_type::virt:
        return is_virtual_operation(op);
    case applied_operation_type::market:
        return is_market_operation(op);
    case applied_operation_type::all:
    default:;
    }

    return true;
}
}
}
