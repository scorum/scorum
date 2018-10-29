#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/chain/dba/db_accessor_factory.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

initializator_context::initializator_context(data_service_factory_i& services,
                                             dba::db_accessor_factory& dba_factory,
                                             const genesis_state_type& genesis_state)
    : _services(services)
    , _dba_factory(dba_factory)
    , _genesis_state(genesis_state)
{
}
}
}
}
