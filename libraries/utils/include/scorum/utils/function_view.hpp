#pragma once
#include <utility>
#include <type_traits>

namespace scorum {
namespace utils {
template <typename> class function_view;
template <typename TReturn, typename... TArgs> class function_view<TReturn(TArgs...)>
{
public:
    template <typename TF>
    function_view(TF&& fn) noexcept
        : _fn_ptr(std::addressof(fn))
    {
        _erased_fn = [](const void* fn_ptr, TArgs... args) -> TReturn {
            using T = std::decay_t<TF>;
            using fun_type = std::conditional_t<std::is_const<decltype(fn)>::value, const T, T>;
            using ptr_type = std::conditional_t<std::is_const<decltype(fn)>::value, const void, void>;

            return (*reinterpret_cast<fun_type*>(const_cast<ptr_type*>(fn_ptr)))(std::forward<TArgs>(args)...);
        };
    }

    TReturn operator()(TArgs... args) const
    {
        return _erased_fn(_fn_ptr, std::forward<TArgs>(args)...);
    }

private:
    const void* _fn_ptr;
    TReturn (*_erased_fn)(const void*, TArgs...);
};
}
}
