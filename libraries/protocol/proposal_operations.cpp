#include <scorum/protocol/proposal_operations.hpp>
#include <scorum/protocol/operation_util_impl.hpp>

void scorum::protocol::operation_validate(const scorum::protocol::proposal_operation& op)
{
    op.visit(scorum::protocol::operation_validate_visitor());
}

scorum::protocol::percent_type
scorum::protocol::operation_get_required_quorum(committee_i& committee_service,
                                                const scorum::protocol::proposal_operation& op)
{
    return op.visit(scorum::protocol::operation_get_required_quorum_visitor(committee_service));
}

DEFINE_OPERATION_SERIALIZATOR(scorum::protocol::proposal_operation)
