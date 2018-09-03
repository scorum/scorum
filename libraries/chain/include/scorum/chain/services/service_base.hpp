#pragma once

#include <functional>

#include <scorum/chain/services/dbs_base.hpp>

#include <limits>

namespace scorum {
namespace chain {

template <class T> struct base_service_i
{
    using object_type = T;
    using modifier_type = std::function<void(object_type&)>;
    using call_type = std::function<void(const object_type&)>;
    using object_cref_type = std::reference_wrapper<const object_type>;

    virtual ~base_service_i()
    {
    }

    virtual const object_type& create(const modifier_type& modifier) = 0;

    virtual void update(const modifier_type& modifier) = 0;

    virtual void update(const object_type& o, const modifier_type& modifier) = 0;

    virtual void remove() = 0;

    virtual void remove(const object_type& o) = 0;

    virtual bool is_exists() const = 0;

    virtual const object_type& get() const = 0;

    virtual const object_type& get(const typename object_type::id_type&) const = 0;
};

template <class service_interface> class dbs_service_base : public dbs_base, public service_interface
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_service_base(database& db)
        : _base_type(db)
    {
    }

    using base_service_type = dbs_service_base;

public:
    using modifier_type = typename service_interface::modifier_type;
    using object_type = typename service_interface::object_type;
    using object_cref_type = typename service_interface::object_cref_type;

    virtual const object_type& create(const modifier_type& modifier) override
    {
        return db_impl().template create<object_type>([&](object_type& o) { modifier(o); });
    }

    virtual void update(const modifier_type& modifier) override
    {
        db_impl().modify(get(), [&](object_type& o) { modifier(o); });
    }

    virtual void update(const object_type& o, const modifier_type& modifier) override
    {
        db_impl().modify(o, [&](object_type& c) { modifier(c); });
    }

    virtual void remove() override
    {
        db_impl().remove(get());
    }

    virtual void remove(const object_type& o) override
    {
        db_impl().remove(o);
    }

    virtual bool is_exists() const override
    {
        return nullptr != db_impl().template find<object_type>();
    }

    virtual const object_type& get() const override
    {
        try
        {
            return db_impl().template get<object_type>();
        }
        FC_CAPTURE_AND_RETHROW()
    }

    virtual const object_type& get(const typename object_type::id_type& id) const override
    {
        try
        {
            return db_impl().template get<object_type>(id);
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class... IndexBy, class Key> const object_type& get_by(const Key& arg) const
    {
        try
        {
            return db_impl().template get<object_type, IndexBy...>(arg);
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class... IndexBy, class Key> const object_type* find_by(const Key& arg) const
    {
        try
        {
            return db_impl().template find<object_type, IndexBy...>(arg);
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class... IndexBy, typename Call> void foreach_by(Call&& call) const
    {
        const auto& idx = db_impl()
                              .template get_index<typename chainbase::get_index_type<object_type>::type>()
                              .indices()
                              .template get<IndexBy...>();
        for (auto it = idx.cbegin(); it != idx.cend(); ++it)
        {
            call(*it);
        }
    }

    template <class... IndexBy, class LowerBounder, class UpperBounder>
    std::vector<object_cref_type> get_range_by(LowerBounder lower, UpperBounder upper) const
    {
        try
        {
            std::vector<object_cref_type> ret;

            const auto& idx = db_impl()
                                  .template get_index<typename chainbase::get_index_type<object_type>::type>()
                                  .indices()
                                  .template get<IndexBy...>();

            auto range = idx.range(lower, upper);

            std::copy(range.first, range.second, std::back_inserter(ret));

            return ret;
        }
        FC_CAPTURE_AND_RETHROW()
    }

    template <class... IndexBy, class LowerBounder, class UpperBounder, class UnaryPredicate>
    std::vector<object_cref_type>
    get_filtered_range_by(LowerBounder lower, UpperBounder upper, UnaryPredicate filter) const
    {
        try
        {
            std::vector<object_cref_type> ret;

            const auto& idx = db_impl()
                                  .template get_index<typename chainbase::get_index_type<object_type>::type>()
                                  .indices()
                                  .template get<IndexBy...>();

            auto range = idx.range(lower, upper);

            std::copy_if(range.first, range.second, std::back_inserter(ret), filter);

            return ret;
        }
        FC_CAPTURE_AND_RETHROW()
    }
};

#define ALL_IDS std::numeric_limits<int64_t>::max()

} // namespace chain
} // namespace scorum
