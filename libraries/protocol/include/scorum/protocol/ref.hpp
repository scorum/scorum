#pragma once
#include <memory>

namespace scorum {
namespace utils {

/*
 * std::reference_wrapper has special semantics in template argument deduction rules so it doesn't work properly with
 * fc::static_variant.
 *
 * Using custom 'utils::ref' type instead which doesn't have such semantics
 */
template <typename T> class ref
{
public:
    // We need default ctor because of the fc::static_variant's visitor
    ref() = default;
    ref(T& inst)
        : _inst(std::addressof(inst))
    {
    }
    ref(const ref&) = default;
    ref(ref&&) = default;
    ref& operator=(const ref&) = default;
    ref& operator=(ref&&) = default;

    operator T&() const
    {
        return *_inst;
    }

    T& get() const
    {
        return *_inst;
    }

private:
    T* _inst;
};

template <typename U> ref<U> make_ref(U& r)
{
    return ref<U>(r);
}
}
}