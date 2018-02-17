#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

initializator_context::initializator_context(data_service_factory_i& _services,
                                             const genesis_state_type& _genesis_state)
    : services(_services)
    , genesis_state(_genesis_state)
{
}
}
}
}
