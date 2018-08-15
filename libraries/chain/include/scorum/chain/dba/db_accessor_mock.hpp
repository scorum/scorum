#pragma once
#include <map>
#include <functional>
#include <boost/any.hpp>
#include <fc/exception/exception.hpp>

namespace scorum {
namespace chain {
namespace dba {

template <typename TObject> struct db_accessor_mock
{
public:
    using object_type = TObject;
    using modifier_type = typename std::function<void(object_type&)>;
    using object_cref_type = std::reference_wrapper<const TObject>;

    const object_type& create(const modifier_type& modifier)
    {
        return invoke(&db_accessor_mock::create, modifier);
    }

    void update(const modifier_type& modifier)
    {
        invoke((void (db_accessor_mock::*)(const modifier_type&)) & db_accessor_mock::update, modifier);
    }

    void update(const object_type& obj, const modifier_type& modifier)
    {
        invoke((void (db_accessor_mock::*)(const object_type&, const modifier_type&)) & db_accessor_mock::update, obj,
               modifier);
    }

    void remove()
    {
        invoke((void (db_accessor_mock::*)()) & db_accessor_mock::remove);
    }

    void remove(const object_type& o)
    {
        invoke((void (db_accessor_mock::*)(const object_type&)) & db_accessor_mock::remove, o);
    }

    bool is_empty() const
    {
        return invoke(&db_accessor_mock::is_empty);
    }

    const object_type& get() const
    {
        return invoke(&db_accessor_mock::get);
    }

    template <class IndexBy, class Key> const object_type& get_by(const Key& arg) const
    {
        return invoke(&db_accessor_mock::get_by<IndexBy, Key>, arg);
    }

    template <class IndexBy, class Key> const object_type* find_by(const Key& arg) const
    {
        return invoke(&db_accessor_mock::find_by<IndexBy, Key>, arg);
    }

    template <class IndexBy, class TLower, class TUpper>
    std::vector<object_cref_type> get_range_by(TLower&& lower, TUpper&& upper) const
    {
        return invoke(&db_accessor_mock::get_range_by<IndexBy, TLower, TUpper>, std::forward<TLower>(lower),
                      std::forward<TUpper>(upper));
    }

public:
    template <typename THandler, typename TRet, typename... TArgs>
    void mock(TRet (db_accessor_mock::*ptr)(TArgs...), THandler&& handler)
    {
        void* addr = (void*&)ptr;
        _handlers.emplace(addr, std::function<TRet(TArgs...)>(std::forward<THandler>(handler)));
    }

    template <typename THandler, typename TRet, typename... TArgs>
    void mock(TRet (db_accessor_mock::*ptr)(TArgs...) const, THandler&& handler)
    {
        void* addr = (void*&)ptr;
        _handlers.emplace(addr, std::function<TRet(TArgs...)>(std::forward<THandler>(handler)));
    }

private:
    template <typename TRet, typename... TArgs, typename... UArgs>
    TRet invoke(TRet (db_accessor_mock::*ptr)(TArgs...), UArgs&&... args)
    {
        void* addr = (void*&)ptr;
        if (_handlers.count(addr))
            return boost::any_cast<std::function<TRet(TArgs...)>>(_handlers.at(addr))(std::forward<UArgs>(args)...);

        FC_ASSERT(false, "wasn't mocked");
    }

    template <typename TRet, typename... TArgs, typename... UArgs>
    TRet invoke(TRet (db_accessor_mock::*ptr)(TArgs...) const, UArgs&&... args) const
    {
        void* addr = (void*&)ptr;
        if (_handlers.count(addr))
            return boost::any_cast<std::function<TRet(TArgs...)>>(_handlers.at(addr))(std::forward<UArgs>(args)...);

        FC_ASSERT(false, "wasn't mocked");
    }

    std::map<void*, boost::any> _handlers;
};
} // dba
} // chain
} // scorum
