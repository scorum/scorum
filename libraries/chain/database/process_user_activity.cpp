#include <scorum/chain/database/process_user_activity.hpp>

namespace scorum {
namespace chain {

namespace database_ns {

using scorum::protocol::public_key_type;

user_activity_context::user_activity_context(data_service_factory_i& services, const signed_transaction& trx)
    : _services(services)
    , _trx(trx)
{
}

void process_user_activity_task::on_apply(user_activity_context& /*ctx*/)
{
    // TODO
}
}
}
}
