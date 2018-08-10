#pragma once
#include <scorum/chain/database/database.hpp>

namespace scorum {
namespace chain {
namespace dba {

template <typename TObject> class db_accessor
{
public:
    explicit db_accessor(scorum::chain::database& db)
        : _db(db)
    {
    }

public:
    using object_type = TObject;
    using modifier_type = typename std::function<void(object_type&)>;
    using object_cref_type = std::reference_wrapper<const TObject>;

    const object_type& create(const modifier_type& modifier)
    {
        return _db.template create<object_type>([&](object_type& o) { modifier(o); });
    }

    void update(const modifier_type& modifier)
    {
        _db.modify(get(), [&](object_type& o) { modifier(o); });
    }

    void update(const object_type& o, const modifier_type& modifier)
    {
        _db.modify(o, [&](object_type& c) { modifier(c); });
    }

    void remove()
    {
        _db.remove(get());
    }

    void remove(const object_type& o)
    {
        _db.remove(o);
    }

    bool is_exists() const
    {
        return nullptr != _db.template find<object_type>();
    }

    const object_type& get() const
    {
        try
        {
            return _db.template get<object_type>();
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class IndexBy, class Key> const object_type& get_by(const Key& arg) const
    {
        try
        {
            return _db.template get<object_type, IndexBy>(arg);
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class IndexBy, class Key> const object_type* find_by(const Key& arg) const
    {
        try
        {
            return _db.template find<object_type, IndexBy>(arg);
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class IndexBy, typename TCall> void foreach_by(TCall&& call) const
    {
        const auto& idx = _db.template get_index<typename chainbase::get_index_type<object_type>::type>()
                              .indices()
                              .template get<IndexBy>();
        for (auto it = idx.cbegin(); it != idx.cend(); ++it)
        {
            call(*it);
        }
    }

    template <class IndexBy, class TLower, class TUpper>
    std::vector<object_cref_type> get_range_by(TLower&& lower, TUpper&& upper) const
    {
        try
        {
            std::vector<object_cref_type> ret;

            const auto& idx = _db.template get_index<typename chainbase::get_index_type<object_type>::type>()
                                  .indices()
                                  .template get<IndexBy>();

            auto range = idx.range(std::forward<TLower>(lower), std::forward<TUpper>(upper));

            std::copy(range.first, range.second, std::back_inserter(ret));

            return ret;
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class IndexBy, class TLower, class TUpper, class TPredicate>
    std::vector<object_cref_type> get_filtered_range_by(TLower&& lower, TUpper&& upper, TPredicate&& filter) const
    {
        try
        {
            std::vector<object_cref_type> ret;

            const auto& idx = _db.template get_index<typename chainbase::get_index_type<object_type>::type>()
                                  .indices()
                                  .template get<IndexBy>();

            auto range = idx.range(std::forward<TLower>(lower), std::forward<TUpper>(upper));

            std::copy_if(range.first, range.second, std::back_inserter(ret), std::forward<TPredicate>(filter));

            return ret;
        }
        FC_CAPTURE_AND_RETHROW()
    }

private:
    scorum::chain::database& _db;
};
}
}
}
