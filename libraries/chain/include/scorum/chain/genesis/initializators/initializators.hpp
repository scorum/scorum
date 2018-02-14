#pragma once

#include <fc/reflect/reflect.hpp>
#include <fc/exception/exception.hpp>

#include <vector>
#include <map>
#include <memory>

namespace scorum {
namespace chain {

class data_service_factory_i;
class genesis_state_type;

namespace genesis {

enum initializators
{
    accounts_initializator = 0,
    founders_initializator,
    witnesses_initializator,
    global_property_initializator,
    rewards_initializator,
    registration_initializator,
    witness_schedule_initializator,
    registration_bonus_initializator,
    steemit_bounty_account_initializator
};

class mark
{
public:
    mark(bool m = false)
        : _m(m)
    {
    }

    operator bool() const
    {
        return _m;
    }

    bool operator!() const
    {
        return !_m;
    }

private:
    bool _m = false;
};

struct initializator
{
    virtual ~initializator()
    {
    }

    virtual initializators get_type() const = 0;

    using initializators_reqired_type = std::vector<initializators>;

    virtual initializators_reqired_type get_reqired_types() const
    {
        return {};
    }

    virtual void apply(data_service_factory_i&, const genesis_state_type&) = 0;
};

class initializators_registry
{
protected:
    explicit initializators_registry(data_service_factory_i& services, const genesis_state_type& genesis_state);

    using initializators_ptr_type = std::pair<mark, std::unique_ptr<initializator>>;
    using initializators_mark_type = std::map<initializators, initializators_ptr_type>;

public:
    template <typename InitializatorType> const initializator& register_initializator(InitializatorType* e)
    {
        FC_ASSERT(e);
        initializators_ptr_type& new_initializator = _initializators[e->get_type()];
        new_initializator.second.reset(e);
        return *e;
    }

    void init(initializators t);
    template <typename InitializatorType> void init(InitializatorType& e)
    {
        init(e.get_type());
    }

private:
    data_service_factory_i& _services;
    const genesis_state_type& _genesis_state;
    initializators_mark_type _initializators;
};
}
}
}

// clang-format off
FC_REFLECT_ENUM(scorum::chain::genesis::initializators,
                (accounts_initializator)
                (founders_initializator)
                (witnesses_initializator)
                (global_property_initializator)
                (rewards_initializator)
                (registration_initializator)
                (witness_schedule_initializator)
                (registration_bonus_initializator)
                (steemit_bounty_account_initializator))
// clang-format on
