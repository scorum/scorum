#pragma once
#include <chainbase/database_index.hpp>
#include <chainbase/segment_manager.hpp>

namespace scorum {
namespace chain {
namespace dba {

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
        return _db_idx.template create<object_type>([&](object_type& o) { modifier(o); });
    }

    void update(const modifier_type& modifier)
    {
        _db_idx.modify(get(), [&](object_type& o) { modifier(o); });
    }

    void update(const object_type& o, const modifier_type& modifier)
    {
        _db_idx.modify(o, [&](object_type& c) { modifier(c); });
    }

    void remove()
    {
        _db_idx.remove(get());
    }

    void remove(const object_type& o)
    {
        _db_idx.remove(o);
    }

    bool is_empty() const
    {
        return nullptr == _db_idx.template find<object_type>();
    }

    const object_type& get() const
    {
        try
        {
            return _db_idx.template get<object_type>();
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class IndexBy, class Key> const object_type& get_by(const Key& arg) const
    {
        try
        {
            return _db_idx.template get<object_type, IndexBy>(arg);
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class IndexBy, class Key> const object_type* find_by(const Key& arg) const
    {
        try
        {
            return _db_idx.template find<object_type, IndexBy>(arg);
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class IndexBy, class TLower, class TUpper>
    std::vector<object_cref_type> get_range_by(TLower&& lower, TUpper&& upper) const
    {
        try
        {
            std::vector<object_cref_type> ret;

            const auto& idx = _db_idx.template get_index<typename chainbase::get_index_type<object_type>::type>()
                                  .indices()
                                  .template get<IndexBy>();

            auto range = idx.range(std::forward<TLower>(lower), std::forward<TUpper>(upper));

            std::copy(range.first, range.second, std::back_inserter(ret));

            return ret;
        }
        FC_CAPTURE_AND_RETHROW()
    }

private:
    chainbase::database_index<chainbase::segment_manager>& _db_idx;
};
}
}
}
