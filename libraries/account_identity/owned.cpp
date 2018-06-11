#include <scorum/account_identity/owned.hpp>

#include <scorum/protocol/authority.hpp>

namespace scorum {
namespace account_identity {

using scorum::protocol::account_name_type;

struct get_owned_account_visitor
{
    get_owned_account_visitor(fc::flat_set<account_name_type>& owned)
        : _owned(owned)
    {
    }
    typedef void result_type;

    template <typename T> void operator()(const T& op)
    {
        op.get_required_posting_authorities(_owned);
        op.get_required_active_authorities(_owned);
        op.get_required_owner_authorities(_owned);
    }

private:
    fc::flat_set<account_name_type>& _owned;
};

void operation_get_owned_accounts(const scorum::protocol::operation& op, fc::flat_set<account_name_type>& result)
{
    get_owned_account_visitor v = get_owned_account_visitor(result);
    op.visit(v);
}

void transaction_get_owned_accounts(const scorum::protocol::transaction& tx, fc::flat_set<account_name_type>& result)
{
    for (const auto& op : tx.operations)
        operation_get_owned_accounts(op, result);
}
}
}
