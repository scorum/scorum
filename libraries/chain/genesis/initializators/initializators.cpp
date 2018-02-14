#include <scorum/chain/genesis/initializators/initializators.hpp>

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

namespace {

using recursive_lock_memo_type = std::map<initializators, mark>;

class recursive_lock_type
{
public:
    explicit recursive_lock_type(initializators t)
        : _t(t)
    {
        FC_ASSERT(!_locks[t], "Initializator ${1} already locked. Re-entry detected.", ("1", t));
        _locks[_t] = true;
    }

    ~recursive_lock_type()
    {
        _locks[_t] = false;
    }

private:
    initializators _t;
    static recursive_lock_memo_type _locks;
};

recursive_lock_memo_type recursive_lock_type::_locks = recursive_lock_memo_type();
}

void initializators_registry::init(initializators t)
{
    recursive_lock_type lock(t);

    initializators_mark_type::iterator it = _initializators.find(t);
    FC_ASSERT(_initializators.end() != it, "Initializator ${1} is not registered.", ("1", t));

    initializators_ptr_type& _initializator = it->second;
    if (!_initializator.first)
    {
        const auto& pinitializator = _initializator.second;
        for (initializators t_reqired : pinitializator->get_reqired_types())
        {
            init(t_reqired);
        }

        dlog("Genesis ${1} is processing.", ("1", t));

        pinitializator->apply(_services, _genesis_state);

        dlog("Genesis ${1} is done.", ("1", t));

        _initializator.first = true;
    }
}
}
}
}
