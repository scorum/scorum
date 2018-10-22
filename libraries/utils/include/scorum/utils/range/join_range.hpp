#pragma once
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

namespace scorum {
namespace utils {
template <typename TLhsIter, typename TRhsIter>
class join_iterator : public boost::iterator_facade<join_iterator<TLhsIter, TRhsIter>,
                                                    typename TLhsIter::value_type,
                                                    boost::forward_traversal_tag,
                                                    typename TLhsIter::reference>
{
public:
    static_assert(std::is_same<typename TLhsIter::value_type, typename TRhsIter::value_type>::value,
                  "Different value types");
    static_assert(std::is_same<typename TLhsIter::reference, typename TRhsIter::reference>::value,
                  "Different ref types");

    join_iterator(TLhsIter lhs_first, TLhsIter lhs_last, TRhsIter rhs_first, TRhsIter rhs_last)
        : _lhs_first(lhs_first)
        , _lhs_last(lhs_last)
        , _rhs_first(rhs_first)
        , _rhs_last(rhs_last)
    {
    }

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        if (_lhs_first != _lhs_last)
            ++_lhs_first;
        else
            ++_rhs_first;
    }

    bool equal(const join_iterator& other) const
    {
        if (_lhs_first != _lhs_last && other._lhs_first != other._lhs_last)
            return _lhs_first == other._lhs_first;
        else
            return _lhs_first == _lhs_last && other._lhs_first == other._lhs_last && _rhs_first == other._rhs_first;
    }

    typename TLhsIter::reference dereference() const
    {
        if (_lhs_first != _lhs_last)
            return *_lhs_first;
        else
            return *_rhs_first;
    }

private:
    TLhsIter _lhs_first;
    TLhsIter _lhs_last;
    TRhsIter _rhs_first;
    TRhsIter _rhs_last;
};

template <typename TRng1, typename TRng2>
class join_range : public boost::iterator_range<join_iterator<typename TRng1::iterator, typename TRng2::iterator>>
{
    using iterator_type = join_iterator<typename TRng1::iterator, typename TRng2::iterator>;
    using base_range_type = boost::iterator_range<iterator_type>;

public:
    template <typename URng1, typename URng2>
    join_range(URng1&& rng1, URng2&& rng2)
        : base_range_type(iterator_type(boost::begin(rng1), boost::end(rng1), boost::begin(rng2), boost::end(rng2)),
                          iterator_type(boost::end(rng1), boost::end(rng1), boost::end(rng2), boost::end(rng2)))
    {
    }
};

template <typename TRng1, typename TRng2> inline auto make_join_range(TRng1&& rng1, TRng2&& rng2)
{
    return join_range<std::decay_t<TRng1>, std::decay_t<TRng2>>(std::forward<TRng1>(rng1), std::forward<TRng2>(rng2));
}

namespace adaptors {
namespace detail {
template <typename TRng> struct joined_holder
{
    joined_holder(TRng& rng)
        : rng(rng)
    {
    }

    TRng& rng;
};
}

template <typename TRng> inline auto joined(TRng& rng)
{
    return detail::joined_holder<TRng>(rng);
}

template <typename TRng1, typename TRng2>
inline auto operator|(TRng1&& rng1, const detail::joined_holder<TRng2>& holder)
{
    return make_join_range(std::forward<TRng1>(rng1), holder.rng);
}
}
}
}
