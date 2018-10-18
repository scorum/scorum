#pragma once
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>
#include <fc/optional.hpp>

namespace scorum {
namespace utils {

template <typename TRangeOfRangesIter>
class flatten_iterator : public boost::iterator_facade<flatten_iterator<TRangeOfRangesIter>,
                                                       typename TRangeOfRangesIter::value_type::iterator::value_type,
                                                       boost::forward_traversal_tag,
                                                       typename TRangeOfRangesIter::value_type::iterator::reference>
{
public:
    flatten_iterator(TRangeOfRangesIter it_first, TRangeOfRangesIter it_last)
        : _it_first(it_first)
        , _it_last(it_last)
    {
    }

private:
    using underlying_value_type = typename TRangeOfRangesIter::value_type;
    using underlying_iterator = typename TRangeOfRangesIter::value_type::iterator;
    friend class boost::iterator_core_access;

    void increment()
    {
        fast_forward();

        if (_underlying_it_first != _underlying_it_last)
        {
            ++(_underlying_it_first.value());
        }
        else
        {
            go_next_underlying_range();
            fast_forward();
        }
    }

    bool equal(const flatten_iterator& other) const
    {
        fast_forward();
        other.fast_forward();

        auto is_eq = _it_first == other._it_first && _underlying_it_first == other._underlying_it_first;
        return is_eq;
    }

    typename underlying_iterator::reference dereference() const
    {
        fast_forward();
        return *(_underlying_it_first.value());
    }

    void go_next_underlying_range() const
    {
        ++_it_first;
        _underlying_it_first.reset();
        _underlying_it_last.reset();
    }

    /**
     * Fast-forward all empty underlying ranges
     */
    void fast_forward() const
    {
        while (_it_first != _it_last)
        {
            if (!_underlying_it_first.valid())
            {
                // initialize underlying iterators
                _underlying_rng = *_it_first;
                _underlying_it_first = std::begin(_underlying_rng);
                _underlying_it_last = std::end(_underlying_rng);
            }
            if (_underlying_it_first != _underlying_it_last)
                break;
            else
                go_next_underlying_range();
        }
    }

private:
    mutable TRangeOfRangesIter _it_first;
    TRangeOfRangesIter _it_last;
    mutable underlying_value_type _underlying_rng;
    mutable fc::optional<underlying_iterator> _underlying_it_first;
    mutable fc::optional<underlying_iterator> _underlying_it_last;
};

template <typename TRngOfRngs>
class flatten_range : public boost::iterator_range<flatten_iterator<typename TRngOfRngs::iterator>>
{
    using iterator_type = flatten_iterator<typename TRngOfRngs::iterator>;
    using base_range_type = boost::iterator_range<iterator_type>;

public:
    template <typename URngOfRngs>
    flatten_range(URngOfRngs&& rng)
        : base_range_type(iterator_type(boost::begin(rng), boost::end(rng)),
                          iterator_type(boost::end(rng), boost::end(rng)))
    {
    }
};

template <typename TRngOfRngs> inline auto make_flatten_range(TRngOfRngs&& rng)
{
    return flatten_range<std::decay_t<TRngOfRngs>>(std::forward<TRngOfRngs>(rng));
}

namespace adaptors {
namespace detail {
struct flatten_adaptor
{
};
}

namespace {
const detail::flatten_adaptor flatten = detail::flatten_adaptor();
}

template <typename TRngOfRngs> inline auto operator|(TRngOfRngs&& rng, const detail::flatten_adaptor& adaptor)
{
    return make_flatten_range(std::forward<TRngOfRngs>(rng));
}
}
}
}
