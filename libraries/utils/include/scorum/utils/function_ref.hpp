namespace scorum {
namespace utils {
template <typename TReturn, typename... TArgs> class function_ref;
template <typename TReturn, typename... TArgs> class function_ref<TReturn(TArgs...)>
{
    template <typename TF>
    function_ref(TF&& fn) noexcept
        : _fn(fn)
    {
        _erased_fn = [](void* fn, TArgs... args) -> TReturn {
            return (*reinterpret_cast<TF*>(fn))(std::forward<TArgs>(args)...);
        };
    }

    TReturn operator()(TArgs... args) const
    {
        return _erased_fn(_fn, std::forward<TArgs>(args)...);
    }

private:
    void* _fn;
    TReturn (*_erased_fn)(void*, TArgs...);
};
}
}
