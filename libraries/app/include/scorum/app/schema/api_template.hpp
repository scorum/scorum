#pragma once

namespace scorum {
namespace app {

template <class T> class api_obj : public T
{
    struct constructor
    {
        void operator()(const T&)
        {
        }
    };

public:
    api_obj()
        : T(constructor(), std::allocator<T>())
    {
    }

    api_obj(const T& other)
        : T(constructor(), std::allocator<T>())
    {
        T& base = static_cast<T&>(*this);
        base = other;
    }
};
}
}
