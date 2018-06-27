#pragma once

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <regex>

namespace bip = boost::interprocess;

template <class T> class wrapper_object
{
public:
    template <class Constructor>
    wrapper_object(bip::managed_shared_memory& shm, Constructor&& constructor)
        : obj(constructor, shm.get_allocator<T>())
    {
    }

    wrapper_object(bip::managed_shared_memory& shm)
        : obj([](const T&) {}, shm.get_allocator<T>())
    {
    }

    T& get()
    {
        return obj;
    }

private:
    T obj;
};

template <typename T> T create_object(bip::managed_shared_memory& shm)
{
    wrapper_object<T> obj(shm);
    return std::move(obj.get());
}

template <typename T, typename C> T create_object(bip::managed_shared_memory& shm, C&& constructor)
{
    wrapper_object<T> obj(shm, constructor);
    return std::move(obj.get());
}

struct shm_remove
{
    shm_remove() = delete;

    shm_remove(const std::string& name)
        : _name(name)
    {
        bip::shared_memory_object::remove(_name.c_str());
    }
    ~shm_remove()
    {
        bip::shared_memory_object::remove(_name.c_str());
    }

private:
    std::string _name;
};

template <typename T> const std::string get_shm_name()
{
    return std::regex_replace(boost::core::demangle(typeid(T).name()), std::regex("::"), "_");
}

struct shared_memory_fixture
{
private:
    shm_remove remover;

public:
    shared_memory_fixture(const std::string& shm_name = get_shm_name<shared_memory_fixture>(), std::size_t size = 10000)
        : remover(shm_name)
        , shm(bip::create_only, shm_name.c_str(), size)
    {
    }

    bip::managed_shared_memory shm;
};
