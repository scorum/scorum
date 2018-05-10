#pragma once

#include <scorum/chain/tasks_base.hpp>

#include <scorum/protocol/transaction.hpp>
#include <scorum/protocol/types.hpp>

namespace scorum {
namespace chain {

namespace database_ns {

using scorum::protocol::signed_transaction;

class user_activity_context
{
public:
    explicit user_activity_context(data_service_factory_i& services, const signed_transaction&);

    data_service_factory_i& services() const
    {
        return _services;
    }

    const signed_transaction& transaction() const
    {
        return _trx;
    }

private:
    data_service_factory_i& _services;
    const signed_transaction& _trx;
};

class process_user_activity_task : public task<user_activity_context>
{
public:
    void on_apply(user_activity_context& ctx);
};
}
}
}
