#pragma once

#include "object_wrapper.hpp"

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

#include <hippomocks.h>
#include "actor.hpp"

using namespace scorum::chain;
using namespace scorum::protocol;

template <typename ServiceInterface> class service_base_wrapper
{
protected:
    const size_t shm_size = 10000;

    MockRepository& _mocks;

    shared_memory_fixture& _shm_fixture;

    ServiceInterface* _service = nullptr;

public:
    using interface_type = ServiceInterface;
    using object_type = typename ServiceInterface::object_type;

    template <typename C>
    service_base_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_, C&& constructor)
        : _mocks(mocks_)
        , _shm_fixture(shm_fixture)
        , object(constructor, shm_fixture.shm.get_allocator<object_type>())
    {
        init();
    }

    service_base_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_)
        : _mocks(mocks_)
        , _shm_fixture(shm_fixture)
        , object([](const object_type&) {}, shm_fixture.shm.get_allocator<object_type>())
    {
        init();
    }

    object_type object;

    ServiceInterface& service()
    {
        return *_service;
    }

private:
    void init()
    {
        _service = _mocks.Mock<ServiceInterface>();

        _mocks.OnCall(_service, ServiceInterface::create)
            .Do([this](const typename ServiceInterface::modifier_type& m) -> const object_type& {
                m(this->object);
                return this->object;
            });

        _mocks
            .OnCallOverload(_service,
                            (void (ServiceInterface::*)(const typename ServiceInterface::modifier_type&))
                                & ServiceInterface::update)
            .Do([this](const typename ServiceInterface::modifier_type& m) { m(this->object); });

        _mocks
            .OnCallOverload(
                _service,
                (void (ServiceInterface::*)(const object_type&, const typename ServiceInterface::modifier_type&))
                    & ServiceInterface::update)
            .Do([this](const object_type& object, const typename ServiceInterface::modifier_type& m) {
                BOOST_ASSERT(object.id == this->object.id);
                m(this->object);
            });

        _mocks.OnCallOverload(_service, (void (ServiceInterface::*)(const object_type&)) & ServiceInterface::remove)
            .Do([this](const object_type&) {});
    }
};

class dynamic_global_property_service_wrapper : public service_base_wrapper<dynamic_global_property_service_i>
{
    using base_class = service_base_wrapper<dynamic_global_property_service_i>;

public:
    template <typename C>
    dynamic_global_property_service_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_, C&& constructor)
        : base_class(shm_fixture, mocks_, constructor)
    {
        init_extension();
    }

    dynamic_global_property_service_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_)
        : base_class(shm_fixture, mocks_)
    {
        init_extension();
    }

    void init_extension()
    {
        _mocks.OnCall(_service, dynamic_global_property_service_i::get_genesis_time).Do([this]() -> fc::time_point_sec {
            if (genesis_time == fc::time_point_sec())
                return this->object.time;
            return genesis_time;
        });
        _mocks.OnCall(_service, dynamic_global_property_service_i::head_block_time).Do([this]() -> fc::time_point_sec {
            return this->object.time;
        });
        _mocks.OnCall(_service, dynamic_global_property_service_i::head_block_num).Do([this]() -> uint32_t {
            return this->object.head_block_number;
        });
    }

    fc::time_point_sec genesis_time;
};

class account_service_wrapper : public service_base_wrapper<account_service_i>
{
    using base_class = service_base_wrapper<account_service_i>;

public:
    account_service_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_)
        : base_class(shm_fixture, mocks_)
    {
        init_extension();
    }

    void init_extension()
    {
        _mocks.OnCall(_service, account_service_i::get_account)
            .Do([this](const account_name_type& name) -> const account_object& {
                const auto& it = this->_actors.find(name);
                BOOST_ASSERT(it != this->_actors.end());
                return it->second;
            });

        _mocks
            .OnCallOverload(
                _service,
                (void (account_service_i::*)(const account_object&, const typename account_service_i::modifier_type&))
                    & account_service_i::update)
            .Do([this](const account_object& object, const typename account_service_i::modifier_type& m) {
                const auto& it = this->_actors.find(object.name);
                BOOST_ASSERT(it != this->_actors.end());
                m(it->second);
            });
    }

    void add_actor(const Actor& actor)
    {
        const auto& it = _actors.find(actor.name);
        BOOST_ASSERT(_actors.end() == it);
        _actors.insert(std::make_pair(actor.name, account_object(
                                                      [&actor](account_object& account) {
                                                          account.name = actor.name;
                                                          account.balance = actor.scr_amount;
                                                          account.scorumpower = actor.sp_amount;
                                                      },
                                                      _shm_fixture.shm.get_allocator<account_object>())));
    }

private:
    std::map<account_name_type, account_object> _actors;
};
