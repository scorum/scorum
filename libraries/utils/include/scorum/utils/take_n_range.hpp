#pragma once

#include <boost/iterator/filter_iterator.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

namespace scorum {
namespace utils {
template <typename TRngIt>
class take_n_iterator : public boost::iterator_adaptor<take_n_iterator<TRngIt>,
                                                       TRngIt,
                                                       const typename boost::iterator_value<TRngIt>::type,
                                                       std::forward_iterator_tag,
                                                       decltype(*std::declval<TRngIt>())>
{
public:
    take_n_iterator(TRngIt it, size_t n)
        : take_n_iterator<TRngIt>::iterator_adaptor_(it)
        , _n(n)
    {
    }

    take_n_iterator()
        : take_n_iterator<TRngIt>::iterator_adaptor_()
        , _n(0)
    {
    }

private:
    friend class boost::iterator_core_access;

    void increment() noexcept
    {
        if (--_n != 0)
            ++this->base_reference();
    }

    bool equal(take_n_iterator that) const noexcept
    {
        return (that._n == 0 && _n == 0) || this->base_reference() == that.base_reference();
    }

    size_t _n;
};

template <typename TRng> class take_n_range : public boost::iterator_range<take_n_iterator<typename TRng::iterator>>
{
    using iterator_type = take_n_iterator<typename TRng::iterator>;
    using base_range_type = boost::iterator_range<iterator_type>;

public:
    template <typename URng>
    take_n_range(URng&& rng, size_t n)
        : base_range_type(iterator_type(boost::begin(rng), n), iterator_type(boost::end(rng), 0))
    {
    }
};

template <typename TRng> inline auto make_take_n_range(TRng&& rng, size_t n)
{
    return take_n_range<std::decay_t<TRng>>(std::forward<TRng>(rng), n);
}

namespace adaptors {
struct take_n
{
    take_n(size_t n)
        : n(n)
    {
    }
    size_t n;
};

template <typename TRng> inline auto operator|(TRng&& rng, const adaptors::take_n& holder)
{
    return make_take_n_range(std::forward<TRng>(rng), holder.n);
}
} // namespace adaptors
} // namespace utils
} // namespace scorum
