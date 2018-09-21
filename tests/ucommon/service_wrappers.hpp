#pragma once

#include "object_wrapper.hpp"

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/schema/budget_objects.hpp>

#include <fc/optional.hpp>

#include <hippomocks.h>
#include "actor.hpp"

namespace service_wrappers {
using namespace scorum::chain;
using namespace scorum::protocol;

using fc::optional;

template <typename ServiceInterface> class service_base_wrapper
{
public:
    using interface_type = ServiceInterface;
    using object_type = typename ServiceInterface::object_type;

protected:
    MockRepository& _mocks;

    shared_memory_fixture& _shm_fixture;

    ServiceInterface* _service = nullptr;

    size_t _next_id = 0;

    std::map<typename object_type::id_type, object_type> _objects_by_id;

public:
    template <typename C>
    service_base_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_, C&& constructor)
        : _mocks(mocks_)
        , _shm_fixture(shm_fixture)
    {
        init();

        service().create(constructor);
    }

    service_base_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_)
        : _mocks(mocks_)
        , _shm_fixture(shm_fixture)
    {
        init();
    }

    ServiceInterface& service()
    {
        return *_service;
    }

    virtual const object_type& create(const typename ServiceInterface::modifier_type& m)
    {
        auto id = ++_next_id;
        auto it_p = _objects_by_id.insert(std::make_pair(id, object_type(
                                                                 [&](object_type& obj) {
                                                                     m(obj);
                                                                     obj.id = id;
                                                                 },
                                                                 _shm_fixture.shm.get_allocator<object_type>())));
        return it_p.first->second;
    }

    virtual void update(const object_type& object, const typename ServiceInterface::modifier_type& m)
    {
        const auto& it = _objects_by_id.find(object.id);
        FC_ASSERT(it != _objects_by_id.end());
        m(it->second);
    }

    virtual void remove(const object_type& object)
    {
        const auto& it = _objects_by_id.find(object.id);
        FC_ASSERT(it != _objects_by_id.end());
        _objects_by_id.erase(it);
    }

    void update(const typename ServiceInterface::modifier_type& m)
    {
        FC_ASSERT(!_objects_by_id.empty());
        update(_objects_by_id.begin()->second, m);
    }

    const object_type& get() const
    {
        FC_ASSERT(!_objects_by_id.empty());
        return _objects_by_id.begin()->second;
    }

    const object_type& get(const typename object_type::id_type& id) const
    {
        const auto& it = _objects_by_id.find(id);
        FC_ASSERT(it != _objects_by_id.end());
        return it->second;
    }

    bool is_exists() const
    {
        return (!_objects_by_id.empty());
    }

    bool is_exists(const typename object_type::id_type& id) const
    {
        return (_objects_by_id.find(id) != _objects_by_id.end());
    }

private:
    void init()
    {
        _service = _mocks.Mock<ServiceInterface>();

        _mocks.OnCall(_service, ServiceInterface::create)
            .Do([this](const typename ServiceInterface::modifier_type& m) -> const object_type& {
                return this->create(m);
            });

        _mocks
            .OnCallOverload(_service,
                            (void (ServiceInterface::*)(const typename ServiceInterface::modifier_type&))
                                & ServiceInterface::update)
            .Do([this](const typename ServiceInterface::modifier_type& m) { this->update(m); });

        _mocks
            .OnCallOverload(
                _service,
                (void (ServiceInterface::*)(const object_type&, const typename ServiceInterface::modifier_type&))
                    & ServiceInterface::update)
            .Do([this](const object_type& object, const typename ServiceInterface::modifier_type& m) {
                this->update(object, m);
            });

        _mocks.OnCallOverload(_service, (void (ServiceInterface::*)(const object_type&)) & ServiceInterface::remove)
            .Do([this](const object_type& object) { this->remove(object); });

        _mocks
            .OnCallOverload(_service,
                            (const object_type& (base_service_i<object_type>::*)() const)
                                & base_service_i<object_type>::get)
            .Do([this]() -> const object_type& { return this->get(); });
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
            {
                FC_ASSERT(!this->_objects_by_id.empty());
                return this->_objects_by_id.begin()->second.time;
            }
            return genesis_time;
        });
        _mocks.OnCall(_service, dynamic_global_property_service_i::head_block_time).Do([this]() -> fc::time_point_sec {
            FC_ASSERT(!this->_objects_by_id.empty());
            return this->_objects_by_id.begin()->second.time;
        });
        _mocks.OnCall(_service, dynamic_global_property_service_i::head_block_num).Do([this]() -> uint32_t {
            FC_ASSERT(!this->_objects_by_id.empty());
            return this->_objects_by_id.begin()->second.head_block_number;
        });
    }

    fc::time_point_sec genesis_time;
};

template <typename BudgetServiceInterface>
class advertising_budget_service_wrapper : public service_base_wrapper<BudgetServiceInterface>
{
    using base_class = service_base_wrapper<BudgetServiceInterface>;

protected:
    using object_type = typename base_class::object_type;

    const object_type& create(const typename BudgetServiceInterface::modifier_type& m) override
    {
        const auto& new_obj = base_class::create(m);
        _index_by_owner[new_obj.owner].insert(new_obj.id);
        return new_obj;
    }

    void update(const object_type& object, const typename BudgetServiceInterface::modifier_type& m) override
    {
        _index_by_owner[object.owner].erase(object.id);
        base_class::update(object, m);
        const auto& updated_object = this->get(object.id);
        _index_by_owner[updated_object.owner].insert(object.id);
    }

    void remove(const object_type& object) override
    {
        _index_by_owner[object.owner].erase(object.id);
        base_class::remove(object);
    }

    typename BudgetServiceInterface::budgets_type get_budgets(const account_name_type& name) const
    {
        const auto& it_by_owner = this->_index_by_owner.find(name);
        typename BudgetServiceInterface::budgets_type ret;
        if (it_by_owner != this->_index_by_owner.end())
        {
            for (const auto& it : it_by_owner->second)
            {
                const auto& it_by_id = this->_objects_by_id.find(it);
                FC_ASSERT(it_by_id != this->_objects_by_id.end());
                ret.push_back(std::cref(it_by_id->second));
            }
        }
        return ret;
    }

    const object_type& get_budget(const account_name_type& name, const typename object_type::id_type& id) const
    {
        const auto& it_by_owner = this->_index_by_owner.find(name);
        typename BudgetServiceInterface::budgets_type ret;
        FC_ASSERT(it_by_owner != this->_index_by_owner.end());

        const auto& it_by_id = this->_objects_by_id.find(id);
        FC_ASSERT(it_by_id != this->_objects_by_id.end());

        return it_by_id->second;
    }

public:
    advertising_budget_service_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_)
        : base_class(shm_fixture, mocks_)
    {
    }

protected:
    std::map<account_name_type, std::set<typename object_type::id_type>> _index_by_owner;
};

class post_budget_service_wrapper : public advertising_budget_service_wrapper<post_budget_service_i>
{
    using base_class = advertising_budget_service_wrapper<post_budget_service_i>;

public:
    post_budget_service_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_)
        : base_class(shm_fixture, mocks_)
    {
        init_extension();
    }

    void init_extension()
    {
        _mocks.OnCall(_service, post_budget_service_i::create_budget)
            .Do([this](const account_name_type& owner, const asset& balance, fc::time_point_sec start,
                       fc::time_point_sec end, const std::string& json_metadata) -> decltype(auto) {
                return this->create([&](post_budget_service_i::object_type& o) {
                    o.owner = owner;
                    o.created = fc::time_point_sec(42);
                    o.cashout_time = o.created + SCORUM_ADVERTISING_CASHOUT_PERIOD_SEC;
                    o.start = start;
                    o.deadline = end;
                    o.balance = balance;
                    o.per_block = asset(42, SCORUM_SYMBOL);
                });
            });

        _mocks
            .OnCallOverload(
                _service,
                (post_budget_service_i::budgets_type(post_budget_service_i::*)(const account_name_type&) const)
                    & post_budget_service_i::get_budgets)
            .Do([this](const account_name_type& name) -> post_budget_service_i::budgets_type {
                return this->get_budgets(name);
            });
        _mocks.OnCall(_service, post_budget_service_i::get_budget)
            .Do([this](const account_name_type& name,
                       const post_budget_object::id_type& id) -> const post_budget_object& {
                return this->get_budget(name, id);
            });
        _mocks
            .OnCallOverload(
                _service,
                (const post_budget_object& (post_budget_service_i::*)(const post_budget_object::id_type&)const)
                    & post_budget_service_i::get)
            .Do([this](const post_budget_object::id_type& id) -> const post_budget_object& { return this->get(id); });
    }
};

class banner_budget_service_wrapper : public advertising_budget_service_wrapper<banner_budget_service_i>
{
    using base_class = advertising_budget_service_wrapper<banner_budget_service_i>;

public:
    banner_budget_service_wrapper(shared_memory_fixture& shm_fixture, MockRepository& mocks_)
        : base_class(shm_fixture, mocks_)
    {
        init_extension();
    }

    void init_extension()
    {
        _mocks.OnCall(_service, banner_budget_service_i::create_budget)
            .Do([this](const account_name_type& owner, const asset& balance, fc::time_point_sec start,
                       fc::time_point_sec end, const std::string& json_metadata) -> decltype(auto) {
                return this->create([&](banner_budget_service_i::object_type& o) {
                    o.owner = owner;
                    o.created = fc::time_point_sec(42);
                    o.cashout_time = o.created + SCORUM_ADVERTISING_CASHOUT_PERIOD_SEC;
                    o.start = start;
                    o.deadline = end;
                    o.balance = balance;
                    o.per_block = asset(42, SCORUM_SYMBOL);
                });
            });
        _mocks
            .OnCallOverload(
                _service,
                (banner_budget_service_i::budgets_type(banner_budget_service_i::*)(const account_name_type&) const)
                    & banner_budget_service_i::get_budgets)
            .Do([this](const account_name_type& name) -> banner_budget_service_i::budgets_type {
                return this->get_budgets(name);
            });
        _mocks.OnCall(_service, banner_budget_service_i::get_budget)
            .Do([this](const account_name_type& name,
                       const banner_budget_object::id_type& id) -> const banner_budget_object& {
                return this->get_budget(name, id);
            });
        _mocks
            .OnCallOverload(
                _service,
                (const banner_budget_object& (banner_budget_service_i::*)(const banner_budget_object::id_type&)const)
                    & banner_budget_service_i::get)
            .Do([this](const banner_budget_object::id_type& id) -> const banner_budget_object& {
                return this->get(id);
            });
    }
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
                const auto& it_by_name = this->_index_by_name.find(name);
                FC_ASSERT(it_by_name != this->_index_by_name.end());
                const auto& it_by_id = this->_objects_by_id.find(it_by_name->second);
                FC_ASSERT(it_by_id != this->_objects_by_id.end());
                return it_by_id->second;
            });

        _mocks
            .OnCallOverload(_service,
                            (void (account_service_i::*)(const account_name_type&, const optional<const char*>&) const)
                                & account_service_i::check_account_existence)
            .Do([this](const account_name_type& name, const optional<const char*>&) {
                const auto& it_by_name = this->_index_by_name.find(name);
                FC_ASSERT(it_by_name != this->_index_by_name.end());
            });

        _mocks.OnCall(_service, account_service_i::increase_balance)
            .Do([this](const account_object& account, const asset& amount) {

                update(account, [&](account_object& obj) { obj.balance += amount; });
            });

        _mocks.OnCall(_service, account_service_i::decrease_balance)
            .Do([this](const account_object& account, const asset& amount) {

                update(account, [&](account_object& obj) { obj.balance -= amount; });
            });

        _mocks.OnCall(_service, account_service_i::increase_scorumpower)
            .Do([this](const account_object& account, const asset& amount) {
                update(account, [&](account_object& obj) { obj.scorumpower += amount; });
            });

        _mocks.OnCall(_service, account_service_i::decrease_scorumpower)
            .Do([this](const account_object& account, const asset& amount) {
                update(account, [&](account_object& obj) { obj.scorumpower -= amount; });
            });

        _mocks.OnCall(_service, account_service_i::create_scorumpower)
            .Do([this](const account_object& account, const asset& amount) -> asset {
                asset new_sp = asset(amount.amount, SP_SYMBOL);
                update(account, [&](account_object& obj) { obj.scorumpower += new_sp; });
                return new_sp;
            });
    }

    void add_actor(const Actor& actor)
    {
        const auto& it = _index_by_name.find(actor.name);
        FC_ASSERT(_index_by_name.end() == it);
        const auto& new_obj = service().create([&actor](account_object& account) {
            account.name = actor.name;
            account.balance = actor.scr_amount;
            account.scorumpower = actor.sp_amount;
        });
        _index_by_name.insert(std::make_pair(actor.name, new_obj.id));
    }

private:
    std::map<account_name_type, account_id_type> _index_by_name;
};
}
