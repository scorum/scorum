#include <scorum/chain/genesis/initializators/initializators.hpp>
#include <fc/exception/exception.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

initializators_registry::initializators_registry(data_service_factory_i& services,
                                                 const genesis_state_type& genesis_state)
    : _services(services)
    , _genesis_state(genesis_state)
{
}

void initializators_registry::init(initializators t)
{
    using recursive_lock_type = std::map<initializators, mark>;
    static recursive_lock_type recursive_lock;
    FC_ASSERT(!recursive_lock[t]);
    recursive_lock[t] = true;
    if (!_initializators_applied[t])
    {
        initializators_ptr_type::iterator it = _initializators_ptr.find(t);
        FC_ASSERT(_initializators_ptr.end() != it);
        const auto& pinitializator = it->second;
        FC_ASSERT(pinitializator);
        for (initializators t_reqired : pinitializator->get_reqired_types())
        {
            init(t_reqired);
        }

        pinitializator->apply(_services, _genesis_state);
        _initializators_applied[t] = true;
    }
    recursive_lock[t] = false;
}
}
}
}
