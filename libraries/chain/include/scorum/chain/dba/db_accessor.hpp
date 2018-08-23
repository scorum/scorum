#pragma once
#include <chainbase/database_index.hpp>
#include <chainbase/segment_manager.hpp>
#include <scorum/utils/function_ref.hpp>

namespace scorum {
namespace chain {
namespace dba {
namespace detail {
// template <typename TObject> using modifier_type = utils::function_ref<void(TObject&)>;
template <typename TObject> using modifier_type = const std::function<void(TObject&)>&;
template <typename TObject> using object_cref_type = std::reference_wrapper<const TObject>;
using db_index = chainbase::database_index<chainbase::segment_manager>;

template <typename TObject> const TObject& create(db_index& db_idx, modifier_type<TObject> modifier)
{
    return db_idx.template create<TObject>([&](TObject& o) { modifier(o); });
}

template <typename TObject> const TObject& get_single(db_index& db_idx)
{
    try
    {
        return db_idx.template get<TObject>();
    }
    FC_CAPTURE_AND_RETHROW()
}

template <typename TObject> void update(db_index& db_idx, const TObject& o, modifier_type<TObject> modifier)
{
    db_idx.modify(o, [&](TObject& o) { modifier(o); });
}

template <typename TObject> void update_single(db_index& db_idx, modifier_type<TObject> modifier)
{
    db_idx.modify(get_single<TObject>(db_idx), [&](TObject& o) { modifier(o); });
}

template <typename TObject> void remove(db_index& db_idx, const TObject& o)
{
    db_idx.remove(o);
}

template <typename TObject> void remove_single(db_index& db_idx)
{
    db_idx.remove(get_single<TObject>(db_idx));
}

template <typename TObject> bool is_empty(db_index& db_idx)
{
    return nullptr == db_idx.template find<TObject>();
}

template <typename TObject, typename IndexBy, typename Key> const TObject& get_by(db_index& db_idx, const Key& arg)
{
    try
    {
        return db_idx.template get<TObject, IndexBy>(arg);
    }
    FC_CAPTURE_AND_RETHROW()
}

template <typename TObject, typename IndexBy, typename Key> const TObject* find_by(db_index& db_idx, const Key& arg)
{
    try
    {
        return db_idx.template find<TObject, IndexBy>(arg);
    }
    FC_CAPTURE_AND_RETHROW()
}

template <typename TObject, class IndexBy, class TLower, class TUpper>
std::vector<object_cref_type<TObject>> get_range_by(db_index& db_idx, TLower&& lower, TUpper&& upper)
{
    try
    {
        std::vector<object_cref_type<TObject>> ret;

        const auto& idx = db_idx.template get_index<typename chainbase::get_index_type<TObject>::type>()
                              .indices()
                              .template get<IndexBy>();

        auto range = idx.range(std::forward<TLower>(lower), std::forward<TUpper>(upper));

        std::copy(range.first, range.second, std::back_inserter(ret));

        return ret;
    }
    FC_CAPTURE_AND_RETHROW()
}
}

template <typename TObject> class db_accessor
{
public:
    explicit db_accessor(chainbase::database_index<chainbase::segment_manager>& db_idx)
        : _db_idx(db_idx)
    {
    }

public:
    using object_type = TObject;
    using modifier_type = typename std::function<void(object_type&)>;
    using object_cref_type = std::reference_wrapper<const TObject>;

    const object_type& create(const modifier_type& modifier)
    {
        return detail::create(_db_idx, modifier);
    }

    void update(const modifier_type& modifier)
    {
        detail::update_single<TObject>(_db_idx, modifier);
    }

    void update(const object_type& o, const modifier_type& modifier)
    {
        detail::update(_db_idx, o, modifier);
    }

    void remove()
    {
        detail::remove_single<TObject>(_db_idx);
    }

    void remove(const object_type& o)
    {
        detail::remove(_db_idx, o);
    }

    bool is_empty() const
    {
        return detail::is_empty<TObject>(_db_idx);
    }

    const object_type& get() const
    {
        return detail::get_single<TObject>(_db_idx);
    }

    template <class IndexBy, class Key> const object_type& get_by(const Key& arg) const
    {
        return detail::get_by<TObject, IndexBy, Key>(_db_idx, arg);
    }

    template <class IndexBy, class Key> const object_type* find_by(const Key& arg) const
    {
        return detail::find_by<TObject, IndexBy, Key>(_db_idx, arg);
    }

    template <class IndexBy, class TLower, class TUpper>
    std::vector<object_cref_type> get_range_by(TLower&& lower, TUpper&& upper) const
    {
        return detail::get_range_by<TObject, IndexBy, TLower, TUpper>(_db_idx, std::forward<TLower>(lower),
                                                                      std::forward<TUpper>(upper));
    }

private:
    chainbase::database_index<chainbase::segment_manager>& _db_idx;
};
}
}
}
